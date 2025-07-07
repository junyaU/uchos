/**
 * @file utils.cpp
 * @brief FAT32 utility functions implementation
 */

#include "file_system/fat.hpp"
#include "graphics/log.hpp"
#include "internal_common.hpp"
#include "memory/page.hpp"
#include "memory/slab.hpp"
#include "task/ipc.hpp"
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <libs/common/message.hpp>
#include <libs/common/process_id.hpp>
#include <libs/common/types.hpp>
#include <vector>

namespace kernel::fs::fat
{

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
		// Ensure dest is null-terminated and has space for extension
		const size_t dest_len = strlen(dest);
		const size_t ext_len = strlen(extension);
		if (dest_len + ext_len < 13) {
			// Safe to use strcat since we've verified the buffer size
			strncat(dest, extension, 13 - dest_len - 1);
		}
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

cluster_t next_cluster(cluster_t cluster_id)
{
	const uint32_t next = FAT_TABLE[cluster_id];
	if (next >= 0x0FFFFFF8UL) {
		return END_OF_CLUSTER_CHAIN;
	}

	return next;
}

directory_entry* find_dir_entry(directory_entry* parent_dir, const char* name)
{
	for (int i = 0; i < ENTRIES_PER_CLUSTER; ++i) {
		if (parent_dir[i].name[0] == 0) {
			break;
		}

		if (entry_name_is_equal(parent_dir[i], name)) {
			return &parent_dir[i];
		}
	}

	return nullptr;
}

directory_entry* find_empty_dir_entry()
{
	if (ROOT_DIR == nullptr) {
		return nullptr;
	}

	for (int i = 0; i < ENTRIES_PER_CLUSTER; ++i) {
		if (ROOT_DIR[i].name[0] == 0) {
			return &ROOT_DIR[i];
		}
	}

	return nullptr;
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

void send_read_req_to_blk_device(unsigned int sector,
								 size_t len,
								 msg_t dst_type,
								 fs_id_t request_id,
								 size_t sequence)
{
	message m = { .type = msg_t::IPC_READ_FROM_BLK_DEVICE,
				  .sender = process_ids::FS_FAT32 };
	m.data.blk_io.sector = sector;
	m.data.blk_io.len = len;
	m.data.blk_io.dst_type = dst_type;
	m.data.blk_io.request_id = request_id;
	m.data.blk_io.sequence = sequence;

	kernel::task::send_message(process_ids::VIRTIO_BLK, m);
}

void send_file_data(fs_id_t id,
					void* buf,
					size_t size,
					ProcessId requester,
					msg_t type,
					bool for_user)
{
	message m = { .type = type, .sender = process_ids::FS_FAT32 };
	m.data.fs.request_id = id;

	if (for_user) {
		void* user_buf = kernel::memory::alloc(size, kernel::memory::ALLOC_ZEROED,
											   memory::PAGE_SIZE);
		if (user_buf == nullptr) {
			LOG_ERROR("failed to allocate memory");
			return;
		}

		memcpy(user_buf, buf, size);
		m.tool_desc.addr = user_buf;
		m.tool_desc.size = size;
		m.tool_desc.present = true;
	} else {
		m.data.fs.buf = buf;
		m.data.fs.len = size;
	}

	kernel::task::send_message(requester, m);
}

} // namespace kernel::fs::fat
