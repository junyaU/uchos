/**
 * @file file.cpp
 * @brief FAT32 file operations implementation
 */

#include "elf.hpp"
#include "fat.hpp"
#include "file_system/file_descriptor.hpp"
#include "file_system/file_info.hpp"
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
		send_file_data(m.data.blk_io.request_id, ctx.buffer.data(), ctx.total_size,
					   ctx.requester, m.type, for_user);
	}

	kernel::memory::free(m.data.blk_io.buf);

	return OK;
}

error_t
process_file_read_request(const message& m, directory_entry* entry, bool for_user)
{
	char file_name[12] = { 0 };
	memcpy(file_name, entry->name, 11);
	file_name[11] = 0;

	file_cache* c = find_file_cache_by_path(file_name);
	if (c != nullptr) {
		send_file_data(m.data.fs.request_id, c->buffer.data(), c->total_size,
					   m.sender, m.type, for_user);
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
		send_read_req_to_blk_device(calc_start_sector(target_cluster),
									BYTES_PER_CLUSTER, m.type, cache->id,
									sequence++);

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

	message sm = { .type = msg_t::IPC_GET_FILE_INFO,
				   .sender = process_ids::FS_FAT32 };
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
			void* buf = kernel::memory::alloc(sizeof(directory_entry),
											  kernel::memory::ALLOC_ZEROED);
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

} // namespace kernel::fs::fat
