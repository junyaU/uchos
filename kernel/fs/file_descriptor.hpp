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
	char name[11]; ///< File name (8.3 format for FAT compatibility)
	size_t size;   ///< File size in bytes
	size_t offset; ///< Current read/write position
};

/**
 * @brief File descriptor entry for process-local management
 *
 * Each process maintains its own table of file descriptor entries.
 * This structure combines the file descriptor data with management
 * information for redirections and usage tracking.
 */
struct file_descriptor_entry {
	bool in_use;		  ///< Whether this entry is currently in use
	file_descriptor fd;	  ///< The actual file descriptor data
	fd_t redirect_to;	  ///< Redirect target (-1 if no redirection)
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

// Process-local file descriptor management functions

/**
 * @brief Initialize a process's file descriptor table
 *
 * Sets up the standard input/output/error descriptors and marks
 * all other entries as unused.
 *
 * @param fd_table Array of file descriptor entries to initialize
 * @param table_size Size of the file descriptor table
 */
void init_process_fd_table(file_descriptor_entry* fd_table, size_t table_size);

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
fd_t allocate_process_fd(file_descriptor_entry* fd_table, size_t table_size,
                        const char* name, size_t size, ProcessId pid);

/**
 * @brief Get a file descriptor entry from a process's table
 *
 * @param fd_table Process's file descriptor table
 * @param table_size Size of the file descriptor table
 * @param fd File descriptor number to look up
 * @return file_descriptor_entry* Pointer to the entry, or nullptr if invalid
 */
file_descriptor_entry* get_process_fd(file_descriptor_entry* fd_table, 
                                     size_t table_size, fd_t fd);

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
error_t release_process_fd(file_descriptor_entry* fd_table, size_t table_size, fd_t fd);

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
error_t copy_fd_table(file_descriptor_entry* dest, const file_descriptor_entry* src,
                     size_t table_size, ProcessId child_pid);

/**
 * @brief Release all file descriptors for a process
 *
 * Called when a process exits to clean up all open files.
 *
 * @param fd_table Process's file descriptor table
 * @param table_size Size of the file descriptor table
 */
void release_all_process_fds(file_descriptor_entry* fd_table, size_t table_size);

} // namespace kernel::fs
