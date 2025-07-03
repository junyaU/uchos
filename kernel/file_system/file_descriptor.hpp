/**
 * @file file_descriptor.hpp
 * @brief File descriptor management for process file access
 */

#pragma once

#include <array>
#include <cstddef>
#include <libs/common/process_id.hpp>
#include <libs/common/types.hpp>

namespace kernel::fs
{

/**
 * @brief File descriptor structure
 *
 * Represents an open file handle for a process. Each file descriptor
 * tracks the file being accessed, the owning process, and the current
 * read/write position.
 */
struct file_descriptor {
	fd_t fd;	   ///< File descriptor number
	char name[11]; ///< File name (8.3 format for FAT compatibility)
	ProcessId pid; ///< Process ID that owns this descriptor
	size_t size;   ///< File size in bytes
	size_t offset; ///< Current read/write position
};

/**
 * @brief Maximum number of file descriptors in the system
 *
 * Limits the total number of open files across all processes.
 */
constexpr size_t MAX_FILE_DESCRIPTORS = 100;

/**
 * @brief Global array of file descriptors
 *
 * System-wide table of all open file descriptors.
 */
extern std::array<file_descriptor, MAX_FILE_DESCRIPTORS> fds;

/**
 * @brief Get a file descriptor by its number
 *
 * @param fd File descriptor number to look up
 * @return file_descriptor* Pointer to the descriptor, or nullptr if invalid
 */
file_descriptor* get_fd(int fd);

/**
 * @brief Register a new file descriptor
 *
 * Creates a new file descriptor for an opened file and assigns it
 * to the specified process.
 *
 * @param name File name (max 10 characters for 8.3 format)
 * @param size File size in bytes
 * @param pid Process ID that owns this descriptor
 * @return file_descriptor* Pointer to the new descriptor, or nullptr if table is
 * full
 */
file_descriptor* register_fd(const char* name, size_t size, ProcessId pid);

/**
 * @brief Initialize the file descriptor table
 *
 * Clears the file descriptor table and prepares it for use.
 * Must be called during kernel initialization.
 */
void init_fds();

} // namespace kernel::fs
