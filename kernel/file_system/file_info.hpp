/**
 * @file file_system/file_info.hpp
 *
 * @brief File Information Structure and Types
 *
 * This file defines the structures, enumerations, and constants used for
 * representing file information across various file systems. It provides
 * a unified interface for handling file attributes, types, and filesystem-specific
 * identifiers, allowing for consistent file manipulation regardless of the
 * underlying file system.
 *
 * Key components include:
 * - File attribute constants
 * - File type enumeration
 * - File system type enumeration
 * - A generic file_info structure for storing comprehensive file metadata
 *
 * This abstraction layer enables the operating system to work with different
 * file systems (such as FAT32, EXT4, NTFS) using a common set of structures
 * and types, promoting modularity and extensibility in file system operations.
 */

#pragma once

#include <cstddef>
#include <cstdint>

constexpr uint32_t ATTR_READ_ONLY = 0x00000001;
constexpr uint32_t ATTR_HIDDEN = 0x00000002;
constexpr uint32_t ATTR_SYSTEM = 0x00000004;
constexpr uint32_t ATTR_DIRECTORY = 0x00000010;
constexpr uint32_t ATTR_ARCHIVE = 0x00000020;
constexpr uint32_t ATTR_NORMAL = 0x00000080;
constexpr uint32_t ATTR_TEMPORARY = 0x00000100;
constexpr uint32_t ATTR_COMPRESSED = 0x00000800;
constexpr uint32_t ATTR_ENCRYPTED = 0x00004000;
constexpr uint32_t ATTR_SPARSE = 0x00000200;

enum class file_type_t : uint8_t {
	REGULAR_FILE = 1,
	DIRECTORY = 2,
	SYMLINK = 3,
	FIFO = 4,
	SOCKET = 5,
	BLOCK_DEVICE = 6,
	CHAR_DEVICE = 7,
	UNKNOWN = 0
};

enum class file_system_type_t : uint8_t {
	FAT32 = 1,
	EXT4 = 2,
	NTFS = 3,
	UNKNOWN = 0
};

struct file_info {
	char name[11];
	size_t size;
	uint32_t attributes;
	uint64_t creation_time;
	uint64_t last_access_time;
	uint64_t last_write_time;
	file_type_t type;
	file_system_type_t fs_type;
	uint64_t fs_id;
};
