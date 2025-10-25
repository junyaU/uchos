/*
 * @file file_system/fat.hpp
 *
 * @brief FAT32 File System
 *
 * This file contains the definition of the `fat` class, which is designed
 * to interact with FAT32 file systems. The FAT32 file system is a widely used
 * file system format for removable drives and digital storage media. This class
 * provides functionalities for reading, writing, and managing files and directories
 * in a FAT32 file system.
 *
 */

#pragma once

#include <cstdint>
#include <libs/common/types.hpp>

namespace kernel::fs
{
struct BiosParameterBlock {
	uint8_t jump_boot[3];
	char oem_name[8];
	uint16_t bytes_per_sector;
	uint8_t sectors_per_cluster;
	uint16_t reserved_sector_count;
	uint8_t num_fats;
	uint16_t root_entry_count;
	uint16_t total_sectors_16;
	uint8_t media;
	uint16_t fat_size_16;
	uint16_t sectors_per_track;
	uint16_t num_heads;
	uint32_t hidden_sectors;
	uint32_t total_sectors_32;
	uint32_t fat_size_32;
	uint16_t ext_flags;
	uint16_t fs_version;
	uint32_t root_cluster;
	uint16_t fs_info;
	uint16_t backup_boot_sector;
	uint8_t reserved[12];
	uint8_t drive_number;
	uint8_t reserved1;
	uint8_t boot_signature;
	uint32_t volume_id;
	char volume_label[11];
	char fs_type[8];
} __attribute__((packed));

enum class entry_attribute : uint8_t {
	READ_ONLY = 0x01,
	HIDDEN = 0x02,
	SYSTEM = 0x04,
	VOLUME_ID = 0x08,
	DIRECTORY = 0x10,
	ARCHIVE = 0x20,
	LONG_NAME = 0x0F,
};

struct DirectoryEntry {
	unsigned char name[11];
	entry_attribute attribute;
	uint8_t ntres;
	uint8_t create_time_tenth;
	uint16_t create_time;
	uint16_t create_date;
	uint16_t last_access_date;
	uint16_t first_cluster_high;
	uint16_t write_time;
	uint16_t write_date;
	uint16_t first_cluster_low;
	uint32_t file_size;

	uint32_t first_cluster() const
	{
		return first_cluster_low | (static_cast<uint32_t>(first_cluster_high) << 16);
	}
} __attribute__((packed));

static const cluster_t END_OF_CLUSTER_CHAIN = 0x0FFFFFFFLU;

constexpr unsigned int BOOT_SECTOR = 0;

// Main task entry point
void fat32_service();

// FAT32 public API
namespace fat
{
// Execute ELF file loaded from FAT32
void execute_file(void* data, const char* name, const char* args);
} // namespace fat

} // namespace kernel::fs
