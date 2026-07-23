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
#include "memory/slab.hpp"

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

/// First byte of DirectoryEntry::name marking the end of a directory's used
/// entries: this slot and all that follow it in the cluster are unused.
constexpr unsigned char DIR_ENTRY_END = 0x00;

/// First byte of DirectoryEntry::name marking a deleted entry: the slot is
/// free for reuse, but later entries in the same cluster may still be valid.
constexpr unsigned char DIR_ENTRY_DELETED = 0xE5;

/// Mask for one 16-bit half (low or high) of a 32-bit cluster number.
constexpr uint32_t CLUSTER_LOW_MASK = 0xFFFF;

/// Bit shift to reach the high half of a 32-bit cluster number.
constexpr unsigned int CLUSTER_HIGH_SHIFT = 16;

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

	/**
	 * @brief Set the first cluster number, splitting it across the low/high fields
	 * @param cluster Cluster number to store
	 */
	void set_first_cluster(uint32_t cluster)
	{
		first_cluster_low = cluster & CLUSTER_LOW_MASK;
		first_cluster_high = (cluster >> CLUSTER_HIGH_SHIFT) & CLUSTER_LOW_MASK;
	}
} __attribute__((packed));

/// This implementation always writes this exact value when it marks the end
/// of a cluster chain in the FAT table.
static const cluster_t END_OF_CLUSTER_CHAIN = 0x0FFFFFFFLU;

/// FAT32 reserves every value from here through END_OF_CLUSTER_CHAIN
/// (0x0FFFFFFF) as an end-of-chain/reserved marker; readers must treat any
/// FAT entry in this range as "no next cluster", even though this
/// implementation only ever writes END_OF_CLUSTER_CHAIN itself when
/// terminating a chain.
static const cluster_t FAT32_EOC_MIN = 0x0FFFFFF8UL;

constexpr unsigned int BOOT_SECTOR = 0;

// Main task entry point
void fat32_service();

} // namespace kernel::fs
