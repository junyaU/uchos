/**
 * @file file_descriptor.hpp
 * @brief File descriptor management for process file access
 */

#pragma once

#include <cstddef>
#include <cstring>
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
	char name[11];  ///< File name (8.3 format for FAT compatibility)
	size_t size;    ///< File size in bytes
	size_t offset;  ///< Current read/write position

	/**
	 * @brief Check if this file descriptor is unused
	 * @return true if the descriptor is not in use (empty name)
	 */
	bool is_unused() const { return name[0] == '\0'; }

	/**
	 * @brief Check if this file descriptor is in use
	 * @return true if the descriptor is in use (non-empty name)
	 */
	bool is_used() const { return name[0] != '\0'; }

	/**
	 * @brief Clear this file descriptor entry
	 *
	 * Resets all fields to their default values, marking the descriptor as unused.
	 */
	void clear()
	{
		name[0] = '\0';
		size = 0;
		offset = 0;
	}

	/**
	 * @brief Check if this file descriptor has the specified name
	 * @param target_name The name to compare against
	 * @return true if the name matches, false otherwise
	 */
	bool has_name(const char* target_name) const { return strcmp(name, target_name) == 0; }
};

// Process-local file descriptor management functions

/**
 * @brief Initialize a process's file descriptor table
 *
 * Sets up the standard input/output/error descriptors and marks
 * all other entries as unused.
 *
 * @param fd_table Array of file descriptors to initialize
 * @param table_size Size of the file descriptor table
 */
void init_process_fd_table(file_descriptor* fd_table, size_t table_size);

/**
 * @brief Allocate a new file descriptor for a process
 *
 * Finds an unused entry in the process's FD table and initializes it
 * with the given file information.
 *
 * @param fd_table Process's file descriptor table
 * @param table_size Size of the file descriptor table
 * @param name File name (max 10 characters for 8.3 format)
 * @param size File size in bytes
 * @param pid Process ID that owns this descriptor
 * @return fd_t The allocated file descriptor number, or NO_FD on failure
 */
fd_t allocate_process_fd(file_descriptor* fd_table,
                         size_t table_size,
                         const char* name,
                         size_t size,
                         ProcessId pid);

/**
 * @brief Get a file descriptor from a process's table
 *
 * @param fd_table Process's file descriptor table
 * @param table_size Size of the file descriptor table
 * @param fd File descriptor number to look up
 * @return file_descriptor* Pointer to the descriptor, or nullptr if invalid
 */
file_descriptor* get_process_fd(file_descriptor* fd_table, size_t table_size, fd_t fd);

/**
 * @brief Release a file descriptor in a process's table
 *
 * Marks the file descriptor as unused and clears its data.
 *
 * @param fd_table Process's file descriptor table
 * @param table_size Size of the file descriptor table
 * @param fd File descriptor number to release
 * @return error_t OK on success, error code on failure
 */
error_t release_process_fd(file_descriptor* fd_table, size_t table_size, fd_t fd);

/**
 * @brief Copy file descriptor table for process forking
 *
 * Creates a deep copy of the parent's file descriptor table for the child process.
 *
 * @param dest Destination FD table (child process)
 * @param src Source FD table (parent process)
 * @param table_size Size of the file descriptor tables
 * @param child_pid Process ID of the child process
 * @return error_t OK on success, error code on failure
 */
error_t copy_fd_table(file_descriptor* dest,
                      const file_descriptor* src,
                      size_t table_size,
                      ProcessId child_pid);

/**
 * @brief Release all file descriptors for a process
 *
 * Called when a process exits to clean up all open files.
 *
 * @param fd_table Process's file descriptor table
 * @param table_size Size of the file descriptor table
 */
void release_all_process_fds(file_descriptor* fd_table, size_t table_size);

}  // namespace kernel::fs
