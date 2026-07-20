/**
 * @file utils.cpp
 * @brief FAT32 utility functions implementation
 */

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <libs/common/message.hpp>
#include <libs/common/process_id.hpp>
#include <libs/common/types.hpp>
#include <utility>
#include "fat.hpp"
#include "internal_common.hpp"
#include "log/log.hpp"
#include "memory/page.hpp"
#include "memory/slab.hpp"
#include "task/ipc.hpp"

namespace kernel::fs::fat
{

void read_dir_entry_name_raw(const DirectoryEntry& entry, char* dest)
{
	char extension[5] = ".";

	memcpy(dest, &entry.name[0], 8);
	dest[8] = 0;

	for (int i = 7; i >= 0 && dest[i] == SFN_PAD; i--) {
		dest[i] = 0;
	}

	memcpy(extension + 1, &entry.name[8], 3);
	extension[4] = 0;
	for (int i = 2; i >= 0 && extension[i + 1] == SFN_PAD; i--) {
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

void read_dir_entry_name(const DirectoryEntry& entry, char* dest)
{
	read_dir_entry_name_raw(entry, dest);

	// Convert to lowercase for display
	for (int i = 0; dest[i] != '\0'; i++) {
		if (dest[i] >= 'A' && dest[i] <= 'Z') {
			dest[i] = dest[i] + ('a' - 'A');
		}
	}
}

bool entry_name_is_equal(const DirectoryEntry& entry, const char* name)
{
	char entry_name[13];
	read_dir_entry_name_raw(entry, entry_name);

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
	if (next >= FAT32_EOC_MIN) {
		return END_OF_CLUSTER_CHAIN;
	}

	return next;
}

DirectoryEntry* find_dir_entry(DirectoryEntry* parent_dir, const char* name)
{
	for (int i = 0; i < ENTRIES_PER_CLUSTER; ++i) {
		if (parent_dir[i].name[0] == DIR_ENTRY_END) {
			break;
		}

		if (entry_name_is_equal(parent_dir[i], name)) {
			return &parent_dir[i];
		}
	}

	return nullptr;
}

DirectoryEntry* find_empty_dir_entry()
{
	if (ROOT_DIR == nullptr) {
		return nullptr;
	}

	for (int i = 0; i < ENTRIES_PER_CLUSTER; ++i) {
		if (ROOT_DIR[i].name[0] == DIR_ENTRY_END) {
			return &ROOT_DIR[i];
		}
	}

	return nullptr;
}

size_t total_fat_clusters()
{
	if (FAT_TABLE == nullptr || VOLUME_BPB == nullptr ||
		VOLUME_BPB->sectors_per_cluster == 0) {
		return 0;
	}

	const size_t total_clusters =
			VOLUME_BPB->total_sectors_32 / VOLUME_BPB->sectors_per_cluster;

	// Cluster numbers above 0x0FFFFFF7 are reserved in FAT32
	return total_clusters < FAT32_EOC_MIN ? total_clusters : FAT32_EOC_MIN;
}

cluster_t extend_cluster_chain(uint32_t* fat,
							   size_t total_clusters,
							   cluster_t last_cluster,
							   int num_clusters)
{
	cluster_t current_cluster = last_cluster;
	int num_allocated = 0;
	for (size_t i = 2; i < total_clusters && num_allocated < num_clusters; ++i) {
		if (fat[i] != 0) {
			continue;
		}

		fat[current_cluster] = i;
		current_cluster = i;
		++num_allocated;
	}

	fat[current_cluster] = END_OF_CLUSTER_CHAIN;

	if (num_allocated < num_clusters) {
		LOG_ERROR("disk full: allocated %d of %d clusters", num_allocated,
				  num_clusters);
		return 0;
	}

	return current_cluster;
}

cluster_t extend_cluster_chain(cluster_t last_cluster, int num_clusters)
{
	return extend_cluster_chain(FAT_TABLE, total_fat_clusters(), last_cluster,
								num_clusters);
}

cluster_t allocate_cluster_chain(uint32_t* fat,
								 size_t total_clusters,
								 size_t num_clusters)
{
	cluster_t first_cluster = 2;
	while (first_cluster < total_clusters && fat[first_cluster] != 0) {
		++first_cluster;
	}

	if (first_cluster >= total_clusters) {
		LOG_ERROR("disk full: no free cluster found");
		return 0;
	}

	fat[first_cluster] = END_OF_CLUSTER_CHAIN;

	if (num_clusters > 1) {
		if (extend_cluster_chain(fat, total_clusters, first_cluster,
								 num_clusters - 1) == 0) {
			// Roll back the partially allocated chain
			free_cluster_chain(fat, first_cluster);
			return 0;
		}
	}

	return first_cluster;
}

cluster_t allocate_cluster_chain(size_t num_clusters)
{
	return allocate_cluster_chain(FAT_TABLE, total_fat_clusters(), num_clusters);
}

size_t count_free_clusters(const uint32_t* fat, size_t total_clusters)
{
	size_t free_count = 0;
	// Start from cluster 2 (0 and 1 are reserved)
	for (size_t i = 2; i < total_clusters; i++) {
		if (fat[i] == 0) {
			free_count++;
		}
	}

	return free_count;
}

size_t count_free_clusters()
{
	return count_free_clusters(FAT_TABLE, total_fat_clusters());
}

void free_cluster_chain(uint32_t* fat, cluster_t start_cluster)
{
	if (start_cluster == 0 || start_cluster >= FAT32_EOC_MIN) {
		return;
	}

	cluster_t current = start_cluster;
	while (current < FAT32_EOC_MIN) {
		cluster_t next = fat[current];
		fat[current] = 0;
		current = next;
	}
}

void free_cluster_chain(cluster_t start_cluster)
{
	free_cluster_chain(FAT_TABLE, start_cluster);
}

kernel::memory::unique_kbuf<> read_from_blk(unsigned int sector, size_t len)
{
	Message m = { .type = MsgType::BLK_READ, .sender = process_ids::FS_FAT32 };
	m.data.blk.sector = sector;
	m.data.blk.len = len;

	const error_t err = kernel::task::call(process_ids::VIRTIO_BLK, &m);
	if (IS_ERR(err) || IS_ERR(m.result) || m.ool.size == 0) {
		LOG_ERROR("block read failed: sector=%u len=%lu err=%d result=%d", sector,
				  len, err, m.result);
		return kernel::memory::unique_kbuf<>{};
	}

	// The reply's OOL buffer moves to us (kernel-to-kernel: same address)
	return kernel::memory::unique_kbuf<>{ reinterpret_cast<void*>(m.ool.addr) };
}

error_t write_to_blk(void* buffer, unsigned int sector, size_t len)
{
	Message m = { .type = MsgType::BLK_WRITE, .sender = process_ids::FS_FAT32 };
	m.data.blk.sector = sector;
	m.data.blk.len = len;
	m.ool.addr = reinterpret_cast<uint64_t>(buffer);
	m.ool.size = len;

	// Ownership of buffer moves to the blk task, which frees it after the
	// device write; delivery failure means it was never handed over.
	const error_t err = kernel::task::call(process_ids::VIRTIO_BLK, &m);
	if (IS_ERR(err)) {
		kernel::memory::free(buffer);
		return err;
	}

	return m.result;
}

void write_fat_sectors_chunked(unsigned int base_sector, size_t sectors_per_fat)
{
	const size_t bytes_per_sector = VOLUME_BPB->bytes_per_sector;
	const size_t sectors_per_chunk = FAT_WRITE_CHUNK_SIZE / bytes_per_sector;

	size_t sector_offset = 0;
	while (sector_offset < sectors_per_fat) {
		const size_t sectors_remaining = sectors_per_fat - sector_offset;
		const size_t current_chunk_sectors = (sectors_remaining < sectors_per_chunk)
													 ? sectors_remaining
													 : sectors_per_chunk;
		const size_t chunk_size = current_chunk_sectors * bytes_per_sector;

		void* chunk_buffer =
				kernel::memory::alloc(chunk_size, kernel::memory::ALLOC_ZEROED);
		if (chunk_buffer == nullptr) {
			// Fall back to sector-sized writes: a 512B buffer can still
			// succeed when the 64KB chunk cannot
			for (size_t i = 0; i < current_chunk_sectors; ++i) {
				void* sector_buffer = kernel::memory::alloc(
						bytes_per_sector, kernel::memory::ALLOC_ZEROED);
				if (sector_buffer == nullptr) {
					LOG_ERROR("Failed to allocate sector buffer for FAT write");
					continue;
				}
				memcpy(sector_buffer,
					   reinterpret_cast<uint8_t*>(FAT_TABLE) +
							   ((sector_offset + i) * bytes_per_sector),
					   bytes_per_sector);
				const error_t err = write_to_blk(
						sector_buffer,
						base_sector + static_cast<unsigned int>(sector_offset + i),
						bytes_per_sector);
				if (IS_ERR(err)) {
					LOG_ERROR("FAT write failed: sector=%u err=%d",
							  base_sector +
									  static_cast<unsigned int>(sector_offset + i),
							  err);
				}
			}
			sector_offset += current_chunk_sectors;
			continue;
		}

		void* fat_chunk = reinterpret_cast<uint8_t*>(FAT_TABLE) +
						  (sector_offset * bytes_per_sector);
		memcpy(chunk_buffer, fat_chunk, chunk_size);

		const error_t err =
				write_to_blk(chunk_buffer, base_sector + sector_offset, chunk_size);
		if (IS_ERR(err)) {
			LOG_ERROR("FAT write failed: sector=%u err=%d",
					  base_sector + static_cast<unsigned int>(sector_offset), err);
		}

		sector_offset += current_chunk_sectors;
	}
}

void write_fat_table_to_disk()
{
	if (FAT_TABLE == nullptr || VOLUME_BPB == nullptr) {
		LOG_ERROR("FAT table or BPB not initialized");
		return;
	}

	const size_t sectors_per_fat = VOLUME_BPB->fat_size_32;

	write_fat_sectors_chunked(FAT_TABLE_SECTOR, sectors_per_fat);

	if (VOLUME_BPB->num_fats > 1) {
		write_fat_sectors_chunked(FAT_TABLE_SECTOR + sectors_per_fat,
								  sectors_per_fat);
	}
}

void reply_file_data(const Message& req, const void* buf, size_t size)
{
	Message m = { .type = req.type, .sender = process_ids::FS_FAT32 };
	m.result = OK;
	m.data.fs.len = size;

	if (size == 0) {
		// Nothing to transfer (e.g. empty file): reply without an OOL
		// buffer so the requester is not left blocking forever.
		kernel::task::reply(req, &m);
		return;
	}

	// Always a fresh copy in an OOL buffer, whether the requester is a user
	// task (mapped at the syscall boundary, released via ool_release) or
	// kernel code like sys_exec (frees it directly). The old cache-borrowed
	// pointer handoff is gone (issue #314 Stage C).
	auto ool_buf = kernel::task::make_ool_buffer(size);
	if (!ool_buf) {
		LOG_ERROR("failed to allocate memory");
		m.data.fs.len = 0;
		m.result = ERR_NO_MEMORY;
		kernel::task::reply(req, &m);
		return;
	}

	memcpy(ool_buf.get(), buf, size);
	kernel::task::reply_with_ool(req, &m, std::move(ool_buf), size);
}

void reply_error(const Message& req, error_t result)
{
	Message m = { .type = req.type, .sender = process_ids::FS_FAT32 };
	m.result = result;
	kernel::task::reply(req, &m);
}

} // namespace kernel::fs::fat
