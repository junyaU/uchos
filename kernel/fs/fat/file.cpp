/**
 * @file file.cpp
 * @brief FAT32 file operations implementation
 */

#include <algorithm>
#include <cstddef>
#include <cstring>
#include <libs/common/message.hpp>
#include <libs/common/process_id.hpp>
#include <libs/common/types.hpp>
#include <utility>
#include "elf.hpp"
#include "fat.hpp"
#include "fs/file_descriptor.hpp"
#include "fs/file_info.hpp"
#include "graphics/font.hpp"
#include "internal_common.hpp"
#include "log/log.hpp"
#include "memory/slab.hpp"
#include "task/ipc.hpp"
#include "task/task.hpp"

namespace kernel::fs::fat
{

namespace
{

/// Build the cache key used by the file cache: the raw 11-byte 8.3 name.
void entry_cache_key(const DirectoryEntry* entry, char (&key)[12])
{
	memcpy(key, entry->name, 11);
	key[11] = '\0';
}

/**
 * @brief Load a file's contents into the file cache, cluster by cluster
 *
 * Each cluster is a synchronous call to the blk service (issue #314
 * Stage B); the old per-request async state machine
 * (request_id/sequence tracking, partial-read progress) is gone.
 *
 * @return The populated cache entry, or nullptr on failure (a partially
 * filled cache is dropped so it can never serve stale data)
 */
FileCache* load_file(const DirectoryEntry* entry, ProcessId requester)
{
	char file_name[12];
	entry_cache_key(entry, file_name);

	FileCache* cached = find_file_cache_by_path(file_name);
	if (cached != nullptr) {
		return cached;
	}

	FileCache* cache = create_file_cache(file_name, entry->file_size, requester);
	if (cache == nullptr) {
		LOG_ERROR("failed to create file cache");
		return nullptr;
	}

	size_t offset = 0;
	cluster_t cluster = entry->first_cluster();
	while (cluster != END_OF_CLUSTER_CHAIN && offset < entry->file_size) {
		auto buf = read_from_blk(calc_start_sector(cluster), BYTES_PER_CLUSTER);
		if (!buf) {
			remove_file_cache_by_path(file_name);
			return nullptr;
		}

		const size_t copy_len =
				std::min(BYTES_PER_CLUSTER, entry->file_size - offset);
		memcpy(cache->buffer.data() + offset, buf.get(), copy_len);
		offset += copy_len;

		cluster = next_cluster(cluster);
	}

	cache->read_size = entry->file_size;

	return cache;
}

/**
 * @brief Read a file and reply to the requester with its contents
 */
void reply_read_result(const Message& req, const DirectoryEntry* entry)
{
	// An empty file has no cluster chain to read; reply immediately so
	// the requester is not left blocking forever.
	if (entry->file_size == 0 || entry->first_cluster() == 0) {
		reply_file_data(req, nullptr, 0);
		return;
	}

	FileCache* cache = load_file(entry, req.sender);
	if (cache == nullptr) {
		reply_error(req, ERR_FAILED_READ_FROM_DEVICE);
		return;
	}

	reply_file_data(req, cache->buffer.data(), cache->total_size);
}

/// Fully resolved fd-based request. entry is non-null iff resolution
/// succeeded; fd is only valid when entry is set.
struct OpenFile {
	FileDescriptor* fd;
	DirectoryEntry* entry;
};

/**
 * @brief Resolve sender task, file descriptor, and directory entry for an
 * fd-based FS request (FS_READ / FS_WRITE)
 *
 * On failure this logs and replies the matching error itself, except when
 * the requester task is already gone — then there is no one to reply to.
 *
 * @return OpenFile with entry != nullptr on success; entry == nullptr means
 * the error was already handled and the caller just returns
 */
OpenFile resolve_open_file(const Message& m)
{
	kernel::task::Task* t = kernel::task::get_task(m.sender);
	if (t == nullptr) {
		LOG_ERROR("Task %d not found - likely already exited", m.sender.raw());
		return {};
	}

	FileDescriptor* fd = kernel::fs::get_process_fd(
			t->fd_table.data(), kernel::task::MAX_FDS_PER_PROCESS, m.data.fs.fd);
	if (fd == nullptr) {
		LOG_ERROR("fd not found");
		reply_error(m, ERR_INVALID_FD);
		return {};
	}

	DirectoryEntry* entry = find_dir_entry(ROOT_DIR, fd->name);
	if (entry == nullptr) {
		LOG_ERROR("entry not found");
		reply_error(m, ERR_NO_FILE);
		return {};
	}

	return { fd, entry };
}

} // namespace

void execute_file(kernel::memory::unique_kbuf<> data,
				  const char* name,
				  const char* args)
{
	exec_elf(std::move(data), name, args);
}

void handle_fs_stat(const Message& m)
{
	const auto* name = m.data.fs.name;
	kernel::graphics::to_upper(const_cast<char*>(name));

	Message sm = { .type = MsgType::FS_STAT, .sender = process_ids::FS_FAT32 };

	const DirectoryEntry* entry = find_dir_entry(ROOT_DIR, name);
	if (entry == nullptr) {
		sm.result = ERR_NO_FILE;
	} else {
		sm.result = OK;
		sm.data.fs.len = entry->file_size;
	}

	kernel::task::reply(m, &sm);
}

void handle_fs_load(const Message& m)
{
	const auto* name = m.data.fs.name;
	kernel::graphics::to_upper(const_cast<char*>(name));

	const DirectoryEntry* entry = find_dir_entry(ROOT_DIR, name);
	if (entry == nullptr) {
		reply_error(m, ERR_NO_FILE);
		return;
	}

	// The reply carries a kernel-owned copy of the file (OOL move); the
	// requester frees it. The old cache-borrowed raw pointer is gone
	// (issue #314 Stage C).
	reply_read_result(m, entry);
}

void handle_fs_open(const Message& m)
{
	Message req = { .type = MsgType::FS_OPEN, .sender = process_ids::FS_FAT32 };
	req.data.fs.fd = -1;

	const char* name = reinterpret_cast<const char*>(m.data.fs.name);
	kernel::graphics::to_upper(const_cast<char*>(name));

	DirectoryEntry* entry = find_dir_entry(ROOT_DIR, name);
	if (entry == nullptr) {
		req.result = ERR_NO_FILE;
		kernel::task::reply(m, &req);
		return;
	}

	// Get the requesting process's task
	kernel::task::Task* t = kernel::task::get_task(m.sender);
	if (t == nullptr) {
		req.result = ERR_INVALID_TASK;
		kernel::task::reply(m, &req);
		return;
	}

	// Allocate a file descriptor in the process's FD table
	fd_t fd = kernel::fs::allocate_process_fd(t->fd_table.data(),
											  kernel::task::MAX_FDS_PER_PROCESS,
											  name, entry->file_size, m.sender);

	req.data.fs.fd = fd;
	req.result = fd < 0 ? ERR_INVALID_FD : OK;
	kernel::task::reply(m, &req);
}

void handle_fs_read(const Message& m)
{
	const OpenFile of = resolve_open_file(m);
	if (of.entry == nullptr) {
		return;
	}

	reply_read_result(m, of.entry);
}

void handle_fs_close(const Message& m)
{
	kernel::task::Task* t = kernel::task::get_task(m.sender);
	if (t == nullptr) {
		LOG_ERROR("Task %d not found in fs_close - likely already exited",
				  m.sender.raw());
		return;
	}

	error_t result = kernel::fs::release_process_fd(
			t->fd_table.data(), kernel::task::MAX_FDS_PER_PROCESS, m.data.fs.fd);
	if (IS_ERR(result)) {
		LOG_ERROR("Failed to close fd %d: error %d", m.data.fs.fd, result);
		return;
	}
}

void handle_fs_write(const Message& m)
{
	// The write payload always arrives OOL (issue #314 Stage C); take
	// ownership immediately so every return path frees it
	kernel::memory::unique_kbuf<> write_buf{ reinterpret_cast<void*>(m.ool.addr) };
	if (!write_buf || m.ool.size == 0) {
		reply_error(m, ERR_INVALID_ARG);
		return;
	}

	// Never trust the inline length beyond the payload actually delivered
	const size_t count = std::min(m.data.fs.len, static_cast<size_t>(m.ool.size));

	const OpenFile of = resolve_open_file(m);
	if (of.entry == nullptr) {
		return;
	}
	FileDescriptor* fd = of.fd;
	DirectoryEntry* entry = of.entry;

	const size_t new_size = fd->offset + count;

	if (entry->first_cluster() == 0) {
		cluster_t new_cluster = allocate_cluster_chain(1);
		if (new_cluster == 0) {
			LOG_ERROR("disk full: no free clusters available");
			reply_error(m, ERR_NO_MEMORY);
			return;
		}

		entry->set_first_cluster(new_cluster);
		write_fat_table_to_disk();
	}

	const size_t current_clusters =
			(entry->file_size + BYTES_PER_CLUSTER - 1) / BYTES_PER_CLUSTER;
	const size_t needed_clusters =
			(new_size + BYTES_PER_CLUSTER - 1) / BYTES_PER_CLUSTER;

	if (needed_clusters > current_clusters) {
		const size_t clusters_to_add = needed_clusters - current_clusters;
		if (count_free_clusters() < clusters_to_add) {
			LOG_ERROR("disk full: need %lu clusters but only %lu free",
					  clusters_to_add, count_free_clusters());
			reply_error(m, ERR_NO_MEMORY);
			return;
		}

		cluster_t last_cluster = entry->first_cluster();
		cluster_t next;
		while ((next = next_cluster(last_cluster)) != END_OF_CLUSTER_CHAIN) {
			last_cluster = next;
		}

		if (extend_cluster_chain(last_cluster, clusters_to_add) == 0) {
			LOG_ERROR("failed to extend cluster chain");
			reply_error(m, ERR_NO_MEMORY);
			return;
		}
		write_fat_table_to_disk();
	}

	const size_t cluster_offset = fd->offset / BYTES_PER_CLUSTER;
	const size_t offset_in_cluster = fd->offset % BYTES_PER_CLUSTER;
	const size_t write_len = std::min(count, BYTES_PER_CLUSTER - offset_in_cluster);

	cluster_t target_cluster = entry->first_cluster();
	for (size_t i = 0; i < cluster_offset && target_cluster != END_OF_CLUSTER_CHAIN;
		 i++) {
		target_cluster = next_cluster(target_cluster);
	}

	if (target_cluster == END_OF_CLUSTER_CHAIN) {
		LOG_ERROR("invalid cluster chain");
		reply_error(m, ERR_INVALID_ARG);
		return;
	}

	auto cluster_buffer = kernel::memory::make_kbuf(BYTES_PER_CLUSTER,
													kernel::memory::ALLOC_ZEROED);
	if (!cluster_buffer) {
		LOG_ERROR("failed to allocate cluster buffer");
		reply_error(m, ERR_NO_MEMORY);
		return;
	}

	// Handle partial cluster writes
	if (offset_in_cluster != 0 || write_len < BYTES_PER_CLUSTER) {
		// For new files or empty files, we can write partial data to zero-filled
		// buffer
		if (entry->file_size == 0 || fd->offset == 0) {
			// Buffer is already zero-filled, just copy the data at the right offset
			memcpy(static_cast<char*>(cluster_buffer.get()) + offset_in_cluster,
				   write_buf.get(), write_len);
		} else {
			// TODO: For existing data, need to read first
			LOG_ERROR(
					"partial cluster writes with existing data not yet implemented");
			reply_error(m, ERR_INVALID_ARG);
			return;
		}
	} else {
		// Full cluster write - copy write data to cluster buffer
		memcpy(cluster_buffer.get(), write_buf.get(), write_len);
	}

	// Invalidate the cached file contents so a read after this write does not
	// return stale data (issue #313).
	char cache_key[12];
	entry_cache_key(entry, cache_key);
	remove_file_cache_by_path(cache_key);

	// Synchronous device write (issue #314 Stage B): the reply carries the
	// real outcome instead of claiming success before the write happened.
	// Ownership of the buffer moves to the blk task, which frees it.
	const error_t write_err =
			write_to_blk(cluster_buffer.release(), calc_start_sector(target_cluster),
						 BYTES_PER_CLUSTER);
	if (IS_ERR(write_err)) {
		LOG_ERROR("block write failed: result=%d", write_err);
		reply_error(m, write_err);
		return;
	}

	fd->offset += write_len;

	if (new_size != entry->file_size) {
		if (new_size < entry->file_size) {
			// File is being truncated, free unused clusters
			const size_t new_clusters =
					(new_size + BYTES_PER_CLUSTER - 1) / BYTES_PER_CLUSTER;

			if (new_clusters < current_clusters) {
				if (new_clusters == 0) {
					// File is now empty, free all clusters
					free_cluster_chain(entry->first_cluster());
					entry->set_first_cluster(0);
				} else {
					// Find the cluster that should be the last one
					cluster_t current = entry->first_cluster();
					for (size_t i = 1;
						 i < new_clusters && current != END_OF_CLUSTER_CHAIN; i++) {
						current = next_cluster(current);
					}

					if (current != END_OF_CLUSTER_CHAIN) {
						// Free clusters after this one
						cluster_t next = next_cluster(current);
						if (next != END_OF_CLUSTER_CHAIN) {
							free_cluster_chain(next);
							FAT_TABLE[current] = END_OF_CLUSTER_CHAIN;
						}
					}
				}
				write_fat_table_to_disk();
			}
		}

		entry->file_size = new_size;
		persist_directory_entry(entry, fd->name);
	}

	Message reply = { .type = MsgType::FS_WRITE, .sender = process_ids::FS_FAT32 };
	reply.result = OK;
	reply.data.fs.len = write_len;
	kernel::task::reply(m, &reply);
}

void handle_fs_mkfile(const Message& m)
{
	Message reply = { .type = MsgType::FS_MKFILE, .sender = process_ids::FS_FAT32 };
	reply.data.fs.fd = NO_FD;

	const char* name = reinterpret_cast<const char*>(m.data.fs.name);
	kernel::graphics::to_upper(const_cast<char*>(name));

	kernel::task::Task* t = kernel::task::get_task(m.sender);
	if (t == nullptr) {
		// Requester is gone; there is no one to reply to.
		LOG_ERROR("Task %d not found - likely already exited", m.sender.raw());
		return;
	}

	DirectoryEntry* existing_entry = find_dir_entry(ROOT_DIR, name);
	if (existing_entry != nullptr) {
		LOG_ERROR("File already exists: %s", name);
		reply.result = ERR_INVALID_ARG;
		kernel::task::reply(m, &reply);
		return;
	}

	DirectoryEntry* entry = find_empty_dir_entry();
	if (entry == nullptr) {
		LOG_ERROR("No empty directory entry found");
		reply.result = ERR_NO_MEMORY;
		kernel::task::reply(m, &reply);
		return;
	}

	const size_t name_len = std::min(strlen(name), sizeof(entry->name));
	memcpy(entry->name, name, name_len);
	entry->attribute = entry_attribute::ARCHIVE;
	entry->file_size = 0;

	fd_t fd = kernel::fs::allocate_process_fd(t->fd_table.data(),
											  kernel::task::MAX_FDS_PER_PROCESS,
											  name, 0, m.sender);

	reply.data.fs.fd = fd;
	reply.result = fd < 0 ? ERR_INVALID_FD : OK;
	kernel::task::reply(m, &reply);
}

void handle_fs_dup2(const Message& m)
{
	Message reply = { .type = MsgType::FS_DUP2, .sender = process_ids::FS_FAT32 };

	fd_t oldfd = m.data.fs.fd;
	fd_t newfd = m.data.fs.operation;

	kernel::task::Task* t = kernel::task::get_task(m.sender);
	if (t == nullptr) {
		reply_error(m, ERR_INVALID_TASK);
		return;
	}

	// TODO: Support for other file descriptors
	if (newfd != STDOUT_FILENO && newfd != STDERR_FILENO) {
		reply_error(m, ERR_INVALID_FD);
		return;
	}

	const error_t dup_err = kernel::fs::dup_process_fd(
			t->fd_table.data(), kernel::task::MAX_FDS_PER_PROCESS, oldfd, newfd);
	if (IS_ERR(dup_err)) {
		reply_error(m, dup_err);
		return;
	}

	reply.result = OK;
	reply.data.fs.fd = newfd;
	kernel::task::reply(m, &reply);
}

} // namespace kernel::fs::fat
