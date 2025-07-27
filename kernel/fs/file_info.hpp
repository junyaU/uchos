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
#include <libs/common/types.hpp>
#include <libs/common/process_id.hpp>
#include <unordered_map>
#include <vector>

namespace kernel::fs
{

/**
 * @brief File attribute constants
 * 
 * Bit flags representing various file attributes. These can be
 * combined using bitwise OR operations.
 * @{
 */
constexpr uint32_t ATTR_READ_ONLY = 0x00000001;   ///< File is read-only
constexpr uint32_t ATTR_HIDDEN = 0x00000002;      ///< File is hidden
constexpr uint32_t ATTR_SYSTEM = 0x00000004;      ///< System file
constexpr uint32_t ATTR_DIRECTORY = 0x00000010;   ///< Entry is a directory
constexpr uint32_t ATTR_ARCHIVE = 0x00000020;     ///< Archive bit (modified since last backup)
constexpr uint32_t ATTR_NORMAL = 0x00000080;      ///< Normal file with no other attributes
constexpr uint32_t ATTR_TEMPORARY = 0x00000100;   ///< Temporary file
constexpr uint32_t ATTR_COMPRESSED = 0x00000800;  ///< File is compressed
constexpr uint32_t ATTR_ENCRYPTED = 0x00004000;   ///< File is encrypted
constexpr uint32_t ATTR_SPARSE = 0x00000200;      ///< Sparse file
/** @} */

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

/**
 * @brief Comprehensive file information structure
 * 
 * Contains all metadata about a file or directory entry.
 */
struct FileInfo {
	char name[11];              ///< File name in 8.3 format
	size_t size;                ///< File size in bytes
	uint32_t attributes;        ///< File attributes (combination of ATTR_* flags)
	uint64_t creation_time;     ///< Creation timestamp
	uint64_t last_access_time;  ///< Last access timestamp
	uint64_t last_write_time;   ///< Last modification timestamp
	file_type_t type;           ///< Type of file (regular, directory, etc.)
	file_system_type_t fs_type; ///< File system type
	uint64_t fs_id;             ///< File system specific identifier
};

/**
 * @brief File cache for buffering file reads
 * 
 * Manages cached file data to optimize file I/O operations.
 * Supports partial reads and tracks read progress.
 */
struct FileCache {
	fs_id_t id;                   ///< Unique cache identifier
	ProcessId requester;          ///< Process that requested this cache
	std::vector<uint8_t> buffer;  ///< Buffer containing file data
	size_t total_size;            ///< Total size of the file
	size_t read_size;             ///< Bytes read so far
	char path[11];                ///< File path

	/**
	 * @brief Construct a new file cache
	 * 
	 * @param total_size Total size of the file to cache
	 * @param id Unique cache identifier
	 * @param requester Process requesting the cache
	 */
	FileCache(size_t total_size, fs_id_t id, ProcessId requester)
		: id(id), requester(requester), total_size(total_size), read_size(0)
	{
		buffer.resize(total_size);
	}

	/**
	 * @brief Check if the entire file has been read
	 * 
	 * @return true if all data has been read into the cache
	 * @return false if more data needs to be read
	 */
	bool is_read_complete() const { return read_size >= total_size; }
};

/**
 * @brief Global map of active file caches
 * 
 * Maps cache IDs to their corresponding file cache objects.
 */
extern std::unordered_map<fs_id_t, FileCache> file_caches;

/**
 * @brief Find a file cache by file path
 * 
 * @param path File path to search for
 * @return file_cache* Pointer to cache if found, nullptr otherwise
 */
FileCache* find_file_cache_by_path(const char* path);

/**
 * @brief Create a new file cache
 * 
 * @param path File path to cache
 * @param total_size Total size of the file
 * @param requester Process requesting the cache
 * @return file_cache* Pointer to the newly created cache
 */
FileCache* create_file_cache(const char* path, size_t total_size, ProcessId requester);

/**
 * @brief Generate a unique file system ID
 * 
 * @return fs_id_t New unique identifier
 */
fs_id_t generate_fs_id();

/**
 * @brief Initialize the file reading context system
 * 
 * Sets up data structures for managing file read operations.
 * Must be called during file system initialization.
 */
void init_read_contexts();

} // namespace kernel::fs
