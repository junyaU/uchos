#include "fat.hpp"
#include "elf.hpp"
#include "file_system/file_info.hpp"
#include "graphics/font.hpp"
#include "graphics/log.hpp"
#include "hardware/virtio/blk.hpp"
#include "libs/common/message.hpp"
#include "memory/slab.hpp"
#include "task/ipc.hpp"
#include "task/task.hpp"
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <libs/common/types.hpp>
#include <queue>
#include <string.h>
#include <utility>
#include <vector>

namespace file_system
{
bios_parameter_block* VOLUME_BPB;
unsigned long BYTES_PER_CLUSTER;
uint32_t* FAT_TABLE;
unsigned int FAT_TABLE_SECTOR;
directory_entry* ROOT_DIR;
std::queue<message> pending_messages;

std::vector<char*> parse_path(const char* path)
{
	std::vector<char*> result;
	if (path == nullptr) {
		return result;
	}

	while (*path != '\0') {
		while (*path == '/') {
			++path;
		}

		if (*path == 0) {
			break;
		}

		result.push_back(const_cast<char*>(path));

		while (*path != '/' && *path != 0) {
			++path;
		}

		if (*path == 0) {
			break;
		}

		*const_cast<char*>(path) = 0;
		++path;
	}

	return result;
}

void read_dir_entry_name(const directory_entry& entry, char* dest)
{
	char extension[5] = ".";

	memcpy(dest, &entry.name[0], 8);
	dest[8] = 0;

	for (int i = 7; i >= 0 && dest[i] == 0x20; i--) {
		dest[i] = 0;
	}

	memcpy(extension + 1, &entry.name[8], 3);
	extension[4] = 0;
	for (int i = 2; i >= 0 && extension[i + 1] == 0x20; i--) {
		extension[i + 1] = 0;
	}

	if (extension[1] != 0) {
		strlcat(dest, extension, 13);
	}
}

bool entry_name_is_equal(const directory_entry& entry, const char* name)
{
	char entry_name[13];
	read_dir_entry_name(entry, entry_name);

	return strcmp(entry_name, name) == 0;
}

unsigned int calc_start_sector(cluster_t cluster_id)
{
	return VOLUME_BPB->reserved_sector_count +
		   VOLUME_BPB->num_fats * VOLUME_BPB->fat_size_32 +
		   (cluster_id - 2) * VOLUME_BPB->sectors_per_cluster;
}

void initialize_fat(void* volume_image)
{
	VOLUME_BPB = reinterpret_cast<bios_parameter_block*>(volume_image);
	BYTES_PER_CLUSTER = static_cast<unsigned long>(VOLUME_BPB->bytes_per_sector) *
						VOLUME_BPB->sectors_per_cluster;
}

cluster_t next_cluster(cluster_t cluster_id)
{
	const uint32_t next = FAT_TABLE[cluster_id];
	if (next >= 0x0FFFFFF8UL) {
		return END_OF_CLUSTER_CHAIN;
	}

	return next;
}

void execute_file(void* data, const char* name, const char* args)
{
	exec_elf(data, name, args);
}

cluster_t extend_cluster_chain(cluster_t last_cluster, int num_clusters)
{
	cluster_t current_cluster = last_cluster;
	size_t num_allocated = 0;
	for (int i = 2; num_allocated < num_clusters; ++i) {
		if (FAT_TABLE[i] != 0) {
			continue;
		}

		FAT_TABLE[current_cluster] = i;
		current_cluster = i;
		++num_allocated;
	}

	FAT_TABLE[current_cluster] = END_OF_CLUSTER_CHAIN;

	return current_cluster;
}

cluster_t allocate_cluster_chain(size_t num_clusters)
{
	cluster_t first_cluster = 2;
	while (FAT_TABLE[first_cluster] != 0) {
		++first_cluster;
	}
	FAT_TABLE[first_cluster] = END_OF_CLUSTER_CHAIN;

	if (num_clusters > 1) {
		extend_cluster_chain(first_cluster, num_clusters - 1);
	}

	return first_cluster;
}

// TODO: fix this
size_t file_descriptor::read(void* buf, size_t len) { return 0; }
size_t file_descriptor::write(const void* buf, size_t len) { return 0; }

void send_read_req_to_blk_device(unsigned int sector,
								 size_t len,
								 int32_t dst_type,
								 bool is_init = false,
								 fs_id_t request_id = 0,
								 size_t sequence = 0)
{
	message m = { .type = IPC_READ_FROM_BLK_DEVICE, .sender = FS_FAT32_TASK_ID };
	m.is_init_message = is_init;
	m.data.blk_io.sector = sector;
	m.data.blk_io.len = len;
	m.data.blk_io.dst_type = dst_type;
	m.data.blk_io.request_id = request_id;
	m.data.blk_io.sequence = sequence;

	send_message(VIRTIO_BLK_TASK_ID, &m);
}

void send_file_data(fs_id_t id, void* buf, size_t len, pid_t requester)
{
	message m = { .type = IPC_READ_FILE_DATA, .sender = FS_FAT32_TASK_ID };
	m.data.blk_io.request_id = id;
	m.data.blk_io.buf = buf;
	m.data.blk_io.len = len;

	send_message(requester, &m);
}

error_t process_read_data_response(const message& m)
{
	auto it = file_caches.find(m.data.blk_io.request_id);
	if (it == file_caches.end()) {
		printk(KERN_ERROR, "request id not found");
		return ERR_INVALID_ARG;
	}

	auto& ctx = it->second;
	size_t offset = m.data.blk_io.sequence * BYTES_PER_CLUSTER;
	size_t copy_len = std::min(BYTES_PER_CLUSTER, ctx.total_size - offset);

	memcpy(ctx.buffer.data() + offset, m.data.blk_io.buf, copy_len);
	ctx.read_size += copy_len;

	if (ctx.is_read_complete()) {
		send_file_data(m.data.blk_io.request_id, ctx.buffer.data(), ctx.total_size,
					   ctx.requester);
	}

	return OK;
}

error_t process_file_read_request(const message& m)
{
	const auto* entry = reinterpret_cast<directory_entry*>(m.data.fs_op.buf);
	if (entry == nullptr) {
		printk(KERN_ERROR, "entry is null");
		return ERR_INVALID_ARG;
	}

	char file_name[12] = { 0 };
	memcpy(file_name, entry->name, 11);
	file_name[11] = 0;

	file_cache* c = find_file_cache_by_path(file_name);
	if (c != nullptr) {
		send_file_data(m.data.fs_op.request_id, c->buffer.data(), c->total_size,
					   m.sender);
		return OK;
	}

	int target_cluster = entry->first_cluster();
	size_t sequence = 0;
	file_cache* cache = create_file_cache(file_name, entry->file_size, m.sender);
	if (cache == nullptr) {
		printk(KERN_ERROR, "failed to create file cache");
		return ERR_NO_MEMORY;
	}

	while (target_cluster != END_OF_CLUSTER_CHAIN) {
		send_read_req_to_blk_device(calc_start_sector(target_cluster),
									BYTES_PER_CLUSTER, IPC_READ_FILE_DATA, false,
									cache->id, sequence++);

		target_cluster = next_cluster(target_cluster);
	}

	return OK;
}

void handle_get_bpb(const message& m)
{
	VOLUME_BPB = reinterpret_cast<bios_parameter_block*>(m.data.blk_io.buf);
	BYTES_PER_CLUSTER = static_cast<unsigned long>(VOLUME_BPB->bytes_per_sector) *
						VOLUME_BPB->sectors_per_cluster;

	FAT_TABLE_SECTOR = VOLUME_BPB->reserved_sector_count;

	size_t table_size = static_cast<size_t>(VOLUME_BPB->fat_size_32) *
						static_cast<size_t>(SECTOR_SIZE);

	send_read_req_to_blk_device(FAT_TABLE_SECTOR, table_size,
								IPC_GET_FAT_TABLE_FAT32, true);
}

void handle_get_fat_table(const message& m)
{
	FAT_TABLE = reinterpret_cast<uint32_t*>(m.data.blk_io.buf);
	unsigned int root_cluster = VOLUME_BPB->root_cluster;
	unsigned int start_sector = calc_start_sector(root_cluster);

	send_read_req_to_blk_device(start_sector, BYTES_PER_CLUSTER,
								IPC_GET_ROOT_DIR_FAT32, true);
}

void handle_get_root_dir(const message& m)
{
	ROOT_DIR = reinterpret_cast<directory_entry*>(m.data.blk_io.buf);
	CURRENT_TASK->is_initilized = true;

	while (!pending_messages.empty()) {
		auto msg = pending_messages.front();
		pending_messages.pop();
		CURRENT_TASK->messages.push(msg);
	}
}

void handle_get_file_info(const message& m)
{
	if (!CURRENT_TASK->is_initilized) {
		pending_messages.push(m);
		return;
	}

	const auto* path = m.data.fs_op.path;
	to_upper(const_cast<char*>(path));

	message sm = { .type = IPC_GET_FILE_INFO, .sender = FS_FAT32_TASK_ID };
	sm.data.fs_op.buf = nullptr;

	const auto entries_per_cluster = BYTES_PER_CLUSTER / sizeof(directory_entry);
	for (int i = 0; i < entries_per_cluster; ++i) {
		if (ROOT_DIR[i].name[0] == 0x00) {
			break;
		}

		if (ROOT_DIR[i].name[0] == 0xE5) {
			continue;
		}

		if (ROOT_DIR[i].attribute == entry_attribute::LONG_NAME) {
			continue;
		}

		if (entry_name_is_equal(ROOT_DIR[i], path)) {
			void* buf = kmalloc(sizeof(directory_entry), KMALLOC_ZEROED);
			memcpy(buf, &ROOT_DIR[i], sizeof(directory_entry));
			sm.data.fs_op.buf = buf;
			break;
		}
	}

	send_message(m.sender, &sm);
}

void handle_read_file_data(const message& m)
{
	if (!CURRENT_TASK->is_initilized) {
		pending_messages.push(m);
		return;
	}

	if (m.sender == VIRTIO_BLK_TASK_ID) {
		process_read_data_response(m);
		return;
	}

	process_file_read_request(m);
}

void fat32_task()
{
	task* t = CURRENT_TASK;
	t->is_initilized = false;
	pending_messages = std::queue<message>();

	init_read_contexts();

	send_read_req_to_blk_device(BOOT_SECTOR, SECTOR_SIZE, IPC_GET_BPB_FAT32, true);

	t->message_handlers[IPC_GET_BPB_FAT32] = handle_get_bpb;
	t->message_handlers[IPC_GET_FAT_TABLE_FAT32] = handle_get_fat_table;
	t->message_handlers[IPC_GET_ROOT_DIR_FAT32] = handle_get_root_dir;
	t->message_handlers[IPC_GET_FILE_INFO] = handle_get_file_info;
	t->message_handlers[IPC_READ_FILE_DATA] = handle_read_file_data;

	process_messages(t);
};

} // namespace file_system
