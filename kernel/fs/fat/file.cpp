/**
 * @file file.cpp
 * @brief FAT32 file operations implementation
 */

#include "elf.hpp"
#include "fat.hpp"
#include "fs/file_descriptor.hpp"
#include "fs/file_info.hpp"
#include "graphics/font.hpp"
#include "graphics/log.hpp"
#include "internal_common.hpp"
#include "memory/slab.hpp"
#include "task/ipc.hpp"
#include "task/task.hpp"
#include <algorithm>
#include <cstddef>
#include <cstring>
#include <libs/common/message.hpp>
#include <libs/common/process_id.hpp>
#include <libs/common/types.hpp>
#include <vector>

namespace kernel::fs::fat
{

error_t process_read_data_response(const Message& m, bool for_user)
{
	auto it = file_caches.find(m.data.blk_io.request_id);
	if (it == file_caches.end()) {
		LOG_ERROR("request id not found");
		return ERR_INVALID_ARG;
	}

	auto& ctx = it->second;
	const size_t offset = m.data.blk_io.sequence * BYTES_PER_CLUSTER;

	// オフセットがファイルサイズを超えている場合は無視
	if (offset >= ctx.total_size) {
		kernel::memory::free(m.data.blk_io.buf);
		return OK;
	}

	const size_t copy_len = std::min(BYTES_PER_CLUSTER, ctx.total_size - offset);

	memcpy(ctx.buffer.data() + offset, m.data.blk_io.buf, copy_len);
	ctx.read_size += copy_len;

	if (ctx.is_read_complete()) {
		send_file_data(m.data.blk_io.request_id,
		               ctx.buffer.data(),
		               ctx.total_size,
		               ctx.requester,
		               m.type,
		               for_user);
	}

	kernel::memory::free(m.data.blk_io.buf);

	return OK;
}

error_t process_file_read_request(const Message& m, DirectoryEntry* entry, bool for_user)
{
	char file_name[12] = { 0 };
	memcpy(file_name, entry->name, 11);
	file_name[11] = 0;

	FileCache* c = find_file_cache_by_path(file_name);
	if (c != nullptr) {
		send_file_data(
		    m.data.fs.request_id, c->buffer.data(), c->total_size, m.sender, m.type, for_user);
		return OK;
	}

	int target_cluster = entry->first_cluster();
	size_t sequence = 0;
	FileCache* cache = create_file_cache(file_name, entry->file_size, m.sender);
	if (cache == nullptr) {
		LOG_ERROR("failed to create file cache");
		return ERR_NO_MEMORY;
	}

	while (target_cluster != END_OF_CLUSTER_CHAIN) {
		send_read_req_to_blk_device(
		    calc_start_sector(target_cluster), BYTES_PER_CLUSTER, m.type, cache->id, sequence++);

		target_cluster = next_cluster(target_cluster);
	}

	return OK;
}

void execute_file(void* data, const char* name, const char* args)
{
	exec_elf(data, name, args);
}

void handle_get_file_info(const Message& m)
{
	if (!kernel::task::CURRENT_TASK->is_initilized) {
		pending_messages.push(m);
		return;
	}

	const auto* name = m.data.fs.name;
	kernel::graphics::to_upper(const_cast<char*>(name));

	Message sm = { .type = MsgType::IPC_GET_FILE_INFO, .sender = process_ids::FS_FAT32 };
	sm.data.fs.buf = nullptr;

	for (int i = 0; i < ENTRIES_PER_CLUSTER; ++i) {
		if (ROOT_DIR[i].name[0] == 0x00) {
			break;
		}

		if (ROOT_DIR[i].name[0] == 0xE5) {
			continue;
		}

		if (ROOT_DIR[i].attribute == entry_attribute::LONG_NAME) {
			continue;
		}

		if (entry_name_is_equal(ROOT_DIR[i], name)) {
			void* buf;
			ALLOC_OR_RETURN(buf, sizeof(DirectoryEntry), kernel::memory::ALLOC_ZEROED);
			memcpy(buf, &ROOT_DIR[i], sizeof(DirectoryEntry));
			sm.data.fs.buf = buf;
			break;
		}
	}

	kernel::task::send_message(m.sender, sm);
}

void handle_read_file_data(const Message& m)
{
	if (!kernel::task::CURRENT_TASK->is_initilized) {
		pending_messages.push(m);
		return;
	}

	if (m.sender == process_ids::VIRTIO_BLK) {
		process_read_data_response(m, false);
		return;
	}

	DirectoryEntry* entry = reinterpret_cast<DirectoryEntry*>(m.data.fs.buf);
	if (entry == nullptr) {
		LOG_ERROR("entry is null");
		return;
	}

	process_file_read_request(m, entry, false);
}

void handle_fs_open(const Message& m)
{
	Message req = { .type = MsgType::FS_OPEN, .sender = process_ids::FS_FAT32 };

	const char* name = reinterpret_cast<const char*>(m.data.fs.name);
	kernel::graphics::to_upper(const_cast<char*>(name));

	DirectoryEntry* entry = find_dir_entry(ROOT_DIR, name);
	if (entry == nullptr) {
		req.data.fs.fd = -1;
		kernel::task::send_message(m.sender, req);
		return;
	}

	// Get the requesting process's task
	kernel::task::Task* t = kernel::task::get_task(m.sender);
	if (t == nullptr) {
		req.data.fs.fd = -1;
		kernel::task::send_message(m.sender, req);
		return;
	}

	// Allocate a file descriptor in the process's FD table
	fd_t fd = kernel::fs::allocate_process_fd(
	    t->fd_table.data(), kernel::task::MAX_FDS_PER_PROCESS, name, entry->file_size, m.sender);

	req.data.fs.fd = fd;
	kernel::task::send_message(m.sender, req);
}

void handle_fs_read(const Message& m)
{
	if (m.sender == process_ids::VIRTIO_BLK) {
		process_read_data_response(m, true);
		return;
	}

	Message req = { .type = MsgType::FS_READ, .sender = process_ids::FS_FAT32 };

	kernel::task::Task* t = kernel::task::get_task(m.sender);
	if (t == nullptr) {
		LOG_ERROR("Task not found: %d", m.sender.raw());
		req.data.fs.len = 0;
		kernel::task::send_message(m.sender, req);
		return;
	}

	FileDescriptor* fd_entry = kernel::fs::get_process_fd(
	    t->fd_table.data(), kernel::task::MAX_FDS_PER_PROCESS, m.data.fs.fd);
	if (fd_entry == nullptr) {
		LOG_ERROR("fd not found");
		req.data.fs.len = 0;
		kernel::task::send_message(m.sender, req);
		return;
	}

	FileDescriptor* fd = fd_entry;

	DirectoryEntry* entry = find_dir_entry(ROOT_DIR, fd->name);
	if (entry == nullptr) {
		LOG_ERROR("entry not found");
		req.data.fs.len = 0;
		kernel::task::send_message(m.sender, req);
		return;
	}

	process_file_read_request(m, entry, true);
}

void handle_fs_close(const Message& m)
{
	kernel::task::Task* t = kernel::task::get_task(m.sender);
	if (t == nullptr) {
		LOG_ERROR("Task %d not found in fs_close - likely already exited", m.sender.raw());
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
	Message reply = { .type = MsgType::FS_WRITE, .sender = process_ids::FS_FAT32 };

	kernel::task::Task* t = kernel::task::get_task(m.sender);
	if (t == nullptr) {
		LOG_ERROR("Task %d not found - likely already exited", m.sender.raw());
		return;
	}

	FileDescriptor* fd_entry = kernel::fs::get_process_fd(
	    t->fd_table.data(), kernel::task::MAX_FDS_PER_PROCESS, m.data.fs.fd);
	if (fd_entry == nullptr) {
		LOG_ERROR("fd not found");
		reply.data.fs.len = 0;
		kernel::task::send_message(m.sender, reply);
		return;
	}

	FileDescriptor* fd = fd_entry;

	DirectoryEntry* entry = find_dir_entry(ROOT_DIR, fd->name);
	if (entry == nullptr) {
		LOG_ERROR("entry not found");
		reply.data.fs.len = 0;
		kernel::task::send_message(m.sender, reply);
		return;
	}

	const size_t new_size = fd->offset + m.data.fs.len;

	if (entry->first_cluster() == 0) {
		if (count_free_clusters() < 1) {
			LOG_ERROR("disk full: no free clusters available");
			reply.data.fs.len = 0;
			kernel::task::send_message(m.sender, reply);
			return;
		}

		cluster_t new_cluster = allocate_cluster_chain(1);
		entry->first_cluster_low = new_cluster & 0xFFFF;
		entry->first_cluster_high = (new_cluster >> 16) & 0xFFFF;
		write_fat_table_to_disk();
	}

	const size_t current_clusters = (entry->file_size + BYTES_PER_CLUSTER - 1) / BYTES_PER_CLUSTER;
	const size_t needed_clusters = (new_size + BYTES_PER_CLUSTER - 1) / BYTES_PER_CLUSTER;

	if (needed_clusters > current_clusters) {
		const size_t clusters_to_add = needed_clusters - current_clusters;
		if (count_free_clusters() < clusters_to_add) {
			LOG_ERROR("disk full: need {} clusters but only {} free",
			          clusters_to_add,
			          count_free_clusters());
			reply.data.fs.len = 0;
			kernel::task::send_message(m.sender, reply);
			return;
		}

		cluster_t last_cluster = entry->first_cluster();
		cluster_t next = last_cluster;
		while ((next = next_cluster(last_cluster)) != END_OF_CLUSTER_CHAIN) {
			last_cluster = next;
		}

		extend_cluster_chain(last_cluster, clusters_to_add);
		write_fat_table_to_disk();
	}

	const size_t cluster_offset = fd->offset / BYTES_PER_CLUSTER;
	const size_t offset_in_cluster = fd->offset % BYTES_PER_CLUSTER;
	const size_t write_len = std::min(m.data.fs.len, BYTES_PER_CLUSTER - offset_in_cluster);

	cluster_t target_cluster = entry->first_cluster();
	for (size_t i = 0; i < cluster_offset && target_cluster != END_OF_CLUSTER_CHAIN; i++) {
		target_cluster = next_cluster(target_cluster);
	}

	if (target_cluster == END_OF_CLUSTER_CHAIN) {
		LOG_ERROR("invalid cluster chain");
		reply.data.fs.len = 0;
		kernel::task::send_message(m.sender, reply);
		return;
	}

	void* cluster_buffer;
	ALLOC_OR_RETURN(cluster_buffer, BYTES_PER_CLUSTER, kernel::memory::ALLOC_ZEROED);

	// Handle partial cluster writes
	if (offset_in_cluster != 0 || write_len < BYTES_PER_CLUSTER) {
		// For new files or empty files, we can write partial data to zero-filled buffer
		if (entry->file_size == 0 || fd->offset == 0) {
			// Buffer is already zero-filled, just copy the data at the right offset
			void* write_data =
			    m.tool_desc.present ? m.tool_desc.addr : const_cast<char*>(m.data.fs.temp_buf);
			memcpy(static_cast<char*>(cluster_buffer) + offset_in_cluster, write_data, write_len);
		} else {
			// TODO: For existing data, need to read first
			LOG_ERROR("partial cluster writes with existing data not yet implemented");
			kernel::memory::free(cluster_buffer);
			reply.data.fs.len = 0;
			kernel::task::send_message(m.sender, reply);
			return;
		}
	} else {
		// Full cluster write - copy write data to cluster buffer
		void* write_data =
		    m.tool_desc.present ? m.tool_desc.addr : const_cast<char*>(m.data.fs.temp_buf);
		memcpy(cluster_buffer, write_data, write_len);
	}

	send_write_req_to_blk_device(cluster_buffer,
	                             calc_start_sector(target_cluster),
	                             BYTES_PER_CLUSTER,
	                             MsgType::FS_WRITE,
	                             m.data.fs.request_id);

	fd->offset += write_len;

	if (new_size != entry->file_size) {
		if (new_size < entry->file_size) {
			// File is being truncated, free unused clusters
			const size_t new_clusters = (new_size + BYTES_PER_CLUSTER - 1) / BYTES_PER_CLUSTER;

			if (new_clusters < current_clusters) {
				if (new_clusters == 0) {
					// File is now empty, free all clusters
					free_cluster_chain(entry->first_cluster(), 0);
					entry->first_cluster_low = 0;
					entry->first_cluster_high = 0;
				} else {
					// Find the cluster that should be the last one
					cluster_t current = entry->first_cluster();
					for (size_t i = 1; i < new_clusters && current != END_OF_CLUSTER_CHAIN; i++) {
						current = next_cluster(current);
					}

					if (current != END_OF_CLUSTER_CHAIN) {
						// Free clusters after this one
						cluster_t next = next_cluster(current);
						if (next != END_OF_CLUSTER_CHAIN) {
							free_cluster_chain(next, 0);
							FAT_TABLE[current] = END_OF_CLUSTER_CHAIN;
						}
					}
				}
				write_fat_table_to_disk();
			}
		}

		entry->file_size = new_size;
		update_directory_entry_on_disk(entry, fd->name);
	}

	// All writes completed successfully, send successful reply
	reply.data.fs.len = write_len;
	kernel::task::send_message(m.sender, reply);

	if (m.tool_desc.present) {
		kernel::memory::free(m.tool_desc.addr);
	}
}

void handle_fs_mkfile(const Message& m)
{
	Message reply = { .type = MsgType::FS_MKFILE, .sender = process_ids::FS_FAT32 };

	const char* name = reinterpret_cast<const char*>(m.data.fs.name);
	kernel::graphics::to_upper(const_cast<char*>(name));

	DirectoryEntry* existing_entry = find_dir_entry(ROOT_DIR, name);
	if (existing_entry != nullptr) {
		LOG_ERROR("File already exists: %s", name);
		return;
	}

	DirectoryEntry* entry = find_empty_dir_entry();
	if (entry == nullptr) {
		LOG_ERROR("No empty directory entry found");
		return;
	}

	memcpy(entry->name, name, strlen(name));
	entry->attribute = entry_attribute::ARCHIVE;
	entry->file_size = 0;

	kernel::task::Task* t = kernel::task::get_task(m.sender);
	if (t == nullptr) {
		LOG_ERROR("Task %d not found - likely already exited", m.sender.raw());
		return;
	}

	fd_t fd = kernel::fs::allocate_process_fd(
	    t->fd_table.data(), kernel::task::MAX_FDS_PER_PROCESS, name, 0, m.sender);

	reply.data.fs.fd = fd;
	kernel::task::send_message(m.sender, reply);
}

void handle_fs_dup2(const Message& m)
{
	Message reply = { .type = MsgType::FS_DUP2, .sender = process_ids::FS_FAT32 };

	fd_t oldfd = m.data.fs.fd;
	fd_t newfd = m.data.fs.operation;

	kernel::task::Task* t = kernel::task::get_task(m.sender);
	if (t == nullptr) {
		reply.data.fs.result = -1;
		kernel::task::send_message(m.sender, reply);
		return;
	}

	kernel::fs::FileDescriptor* old_entry =
	    kernel::fs::get_process_fd(t->fd_table.data(), kernel::task::MAX_FDS_PER_PROCESS, oldfd);
	if (old_entry == nullptr) {
		reply.data.fs.result = -1;
		kernel::task::send_message(m.sender, reply);
		return;
	}

	// TODO: Support for other file descriptors
	if (newfd != STDOUT_FILENO && newfd != STDERR_FILENO) {
		reply.data.fs.result = -1;
		kernel::task::send_message(m.sender, reply);
		return;
	}

	if (newfd >= 0 && newfd < kernel::task::MAX_FDS_PER_PROCESS) {
		t->fd_table[newfd] = *old_entry;
	}

	reply.data.fs.result = newfd;
	reply.data.fs.fd = oldfd;
	kernel::task::send_message(m.sender, reply);
}

}  // namespace kernel::fs::fat
