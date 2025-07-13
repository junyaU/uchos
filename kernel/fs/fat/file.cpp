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

error_t process_read_data_response(const message& m, bool for_user)
{
	auto it = file_caches.find(m.data.blk_io.request_id);
	if (it == file_caches.end()) {
		LOG_ERROR("request id not found");
		return ERR_INVALID_ARG;
	}

	auto& ctx = it->second;
	const size_t offset = m.data.blk_io.sequence * BYTES_PER_CLUSTER;
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

error_t process_file_read_request(const message& m, directory_entry* entry, bool for_user)
{
	char file_name[12] = { 0 };
	memcpy(file_name, entry->name, 11);
	file_name[11] = 0;

	file_cache* c = find_file_cache_by_path(file_name);
	if (c != nullptr) {
		send_file_data(
		    m.data.fs.request_id, c->buffer.data(), c->total_size, m.sender, m.type, for_user);
		return OK;
	}

	int target_cluster = entry->first_cluster();
	size_t sequence = 0;
	file_cache* cache = create_file_cache(file_name, entry->file_size, m.sender);
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

void handle_get_file_info(const message& m)
{
	if (!kernel::task::CURRENT_TASK->is_initilized) {
		pending_messages.push(m);
		return;
	}

	const auto* name = m.data.fs.name;
	kernel::graphics::to_upper(const_cast<char*>(name));

	message sm = { .type = msg_t::IPC_GET_FILE_INFO, .sender = process_ids::FS_FAT32 };
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
			void* buf =
			    kernel::memory::alloc(sizeof(directory_entry), kernel::memory::ALLOC_ZEROED);
			memcpy(buf, &ROOT_DIR[i], sizeof(directory_entry));
			sm.data.fs.buf = buf;
			break;
		}
	}

	kernel::task::send_message(m.sender, sm);
}

void handle_read_file_data(const message& m)
{
	if (!kernel::task::CURRENT_TASK->is_initilized) {
		pending_messages.push(m);
		return;
	}

	if (m.sender == process_ids::VIRTIO_BLK) {
		process_read_data_response(m, false);
		return;
	}

	directory_entry* entry = reinterpret_cast<directory_entry*>(m.data.fs.buf);
	if (entry == nullptr) {
		LOG_ERROR("entry is null");
		return;
	}

	process_file_read_request(m, entry, false);
}

void handle_fs_open(const message& m)
{
	message req = { .type = msg_t::FS_OPEN, .sender = process_ids::FS_FAT32 };

	const char* name = reinterpret_cast<const char*>(m.data.fs.name);
	kernel::graphics::to_upper(const_cast<char*>(name));

	directory_entry* entry = find_dir_entry(ROOT_DIR, name);
	if (entry == nullptr) {
		req.data.fs.fd = -1;
		kernel::task::send_message(m.sender, req);
		return;
	}

	file_descriptor* fd = register_fd(name, entry->file_size, m.sender);
	req.data.fs.fd = fd == nullptr ? -1 : fd->fd;

	kernel::task::send_message(m.sender, req);
}

void handle_fs_read(const message& m)
{
	if (m.sender == process_ids::VIRTIO_BLK) {
		process_read_data_response(m, true);
		return;
	}

	message req = { .type = msg_t::FS_READ, .sender = process_ids::FS_FAT32 };

	file_descriptor* fd = get_fd(m.data.fs.fd);
	if (fd == nullptr) {
		LOG_ERROR("fd not found");
		req.data.fs.len = 0;
		kernel::task::send_message(m.sender, req);
		return;
	}

	directory_entry* entry = find_dir_entry(ROOT_DIR, fd->name);
	if (entry == nullptr) {
		LOG_ERROR("entry not found");
		req.data.fs.len = 0;
		kernel::task::send_message(m.sender, req);
		return;
	}

	process_file_read_request(m, entry, true);
}

void handle_fs_close(const message& m)
{
	file_descriptor* fd = get_fd(m.data.fs.fd);
	if (fd == nullptr) {
		LOG_ERROR("fd not found");
		return;
	}

	memset(fd, 0, sizeof(file_descriptor));
	fd->fd = -1;
}

void handle_fs_write(const message& m)
{
	message reply = { .type = msg_t::FS_WRITE, .sender = process_ids::FS_FAT32 };

	file_descriptor* fd = get_fd(m.data.fs.fd);
	if (fd == nullptr) {
		LOG_ERROR("fd not found");
		reply.data.fs.len = 0;
		kernel::task::send_message(m.sender, reply);
		return;
	}

	directory_entry* entry = find_dir_entry(ROOT_DIR, fd->name);
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

	void* cluster_buffer = kernel::memory::alloc(BYTES_PER_CLUSTER, kernel::memory::ALLOC_ZEROED);
	if (cluster_buffer == nullptr) {
		LOG_ERROR("failed to allocate cluster buffer");
		reply.data.fs.len = 0;
		kernel::task::send_message(m.sender, reply);
		return;
	}

	// Read existing cluster data first (if not writing full cluster)
	if (offset_in_cluster != 0 || write_len < BYTES_PER_CLUSTER) {
		// Need to read existing data first
		// TODO: Implement read-modify-write for partial cluster writes
		LOG_ERROR("partial cluster writes not yet implemented");
		kernel::memory::free(cluster_buffer);
		reply.data.fs.len = 0;
		kernel::task::send_message(m.sender, reply);
		return;
	}

	// Copy write data to cluster buffer
	void* write_data = m.tool_desc.present ? m.tool_desc.addr : m.data.fs.buf;
	memcpy(cluster_buffer, write_data, write_len);

	send_write_req_to_blk_device(cluster_buffer,
	                             calc_start_sector(target_cluster),
	                             BYTES_PER_CLUSTER,
	                             msg_t::FS_WRITE,
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

	reply.data.fs.len = write_len;
	kernel::task::send_message(m.sender, reply);

	// DO NOT free the cluster_buffer here - it will be freed when write completes
	// The buffer is being used by the block device for asynchronous write
	// TODO: Implement proper write completion handling

	if (m.tool_desc.present) {
		kernel::memory::free(m.tool_desc.addr);
	}
}

void handle_fs_mkfile(const message& m)
{
	message reply = { .type = msg_t::FS_MKFILE, .sender = process_ids::FS_FAT32 };

	const char* name = reinterpret_cast<const char*>(m.data.fs.name);
	kernel::graphics::to_upper(const_cast<char*>(name));

	directory_entry* existing_entry = find_dir_entry(ROOT_DIR, name);
	if (existing_entry != nullptr) {
		reply.data.fs.fd = -1;
		kernel::task::send_message(m.sender, reply);
		return;
	}

	directory_entry* entry = find_empty_dir_entry();
	if (entry == nullptr) {
		reply.data.fs.fd = -1;
		kernel::task::send_message(m.sender, reply);
		return;
	}

	memcpy(entry->name, name, strlen(name));
	entry->attribute = entry_attribute::ARCHIVE;
	entry->file_size = 0;

	file_descriptor* fd = register_fd(name, 0, m.sender);

	reply.data.fs.fd = fd == nullptr ? -1 : fd->fd;

	kernel::task::send_message(m.sender, reply);
}

void handle_fs_dup2(const message& m)
{
	message reply = { .type = msg_t::FS_DUP2, .sender = process_ids::FS_FAT32 };

	fd_t oldfd = m.data.fs.fd;
	fd_t newfd = m.data.fs.operation;

	// For simplicity, we'll handle stdout redirection by updating the process's
	// write messages to go to the file instead of the terminal.
	// This is a simplified implementation for UCHos.

	// Check if oldfd is a valid file descriptor
	file_descriptor* fd = get_fd(oldfd);
	if (fd == nullptr || fd->pid != m.sender) {
		reply.data.fs.result = -1;
		kernel::task::send_message(m.sender, reply);
		return;
	}

	// TODO: Support for other file descriptors
	if (newfd != STDOUT_FILENO) {
		reply.data.fs.result = -1;
		kernel::task::send_message(m.sender, reply);
		return;
	}

	kernel::task::task* t = kernel::task::get_task(m.sender);
	if (t == nullptr) {
		reply.data.fs.result = -1;
		kernel::task::send_message(m.sender, reply);
		return;
	}

	t->fd_table[newfd] = oldfd;

	// Success - the actual redirection will be handled by the syscall layer
	// when write(1, ...) is called
	reply.data.fs.result = newfd;
	reply.data.fs.fd = oldfd;  // Store the file fd for later use
	kernel::task::send_message(m.sender, reply);
}

}  // namespace kernel::fs::fat
