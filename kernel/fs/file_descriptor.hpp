/**
 * @file file_descriptor.hpp
 * @brief File descriptor management for process file access
 */

#pragma once

#include <cstddef>
#include <libs/common/file_descriptor.hpp>
#include <libs/common/process_id.hpp>
#include <libs/common/types.hpp>

namespace kernel::fs
{

/// The type lives in libs/common because the entry is part of the
/// kernel/service contract (issue #315). The table itself stays in the
/// kernel: services edit other tasks' entries only through the KERNEL
/// task's FD_INSTALL / FD_REMOVE / FD_DUP requests.
using FileDescriptor = ::FileDescriptor;

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
void init_process_fd_table(FileDescriptor* fd_table, size_t table_size);

/**
 * @brief Allocate a new file descriptor for a process
 *
 * Finds an unused entry in the process's FD table and initializes it
 * with the given file information.
 *
 * @param fd_table Process's file descriptor table
 * @param table_size Size of the file descriptor table
 * @param name File name label (max 12 characters for 8.3 format)
 * @param handle Server-side open-file object id stored in the entry
 * @param dest Service that issued the handle; I/O on the entry is routed
 * to it
 * @return fd_t The allocated file descriptor number, or NO_FD on failure
 */
fd_t allocate_process_fd(FileDescriptor* fd_table,
						 size_t table_size,
						 const char* name,
						 uint32_t handle,
						 ProcessId dest);

/**
 * @brief Get a file descriptor from a process's table
 *
 * @param fd_table Process's file descriptor table
 * @param table_size Size of the file descriptor table
 * @param fd File descriptor number to look up
 * @return FileDescriptor* Pointer to the descriptor, or nullptr if invalid
 */
FileDescriptor* get_process_fd(FileDescriptor* fd_table, size_t table_size, fd_t fd);

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
error_t release_process_fd(FileDescriptor* fd_table, size_t table_size, fd_t fd);

/**
 * @brief Duplicate oldfd's entry onto newfd (dup2 semantics)
 *
 * Overwrites newfd's entry with a copy of oldfd's; any previous state of
 * newfd is discarded without being released (entries hold no resources).
 *
 * @param fd_table Process's file descriptor table
 * @param table_size Size of the file descriptor table
 * @param oldfd Descriptor to duplicate (must be in use)
 * @param newfd Descriptor slot to overwrite
 * @return error_t OK on success, ERR_INVALID_FD when oldfd is not in use or
 * newfd is out of range
 */
error_t dup_process_fd(FileDescriptor* fd_table,
					   size_t table_size,
					   fd_t oldfd,
					   fd_t newfd);

/**
 * @brief Release an entry on behalf of the service it routes to
 *
 * A service may only remove entries that route back to itself, so one
 * service cannot close descriptors another service handed out.
 *
 * @param fd_table Process's file descriptor table
 * @param table_size Size of the file descriptor table
 * @param fd File descriptor number to release
 * @param owner Service asking for the removal
 * @param out_handle Receives the removed entry's handle on success
 * @return error_t OK on success, ERR_INVALID_FD when fd is unused, a
 * standard descriptor, or routed to a different service
 */
error_t remove_owned_fd(FileDescriptor* fd_table,
						size_t table_size,
						fd_t fd,
						ProcessId owner,
						uint32_t* out_handle);

/**
 * @brief dup2 on behalf of the service oldfd routes to
 *
 * The source entry must route to owner. The overwritten entry may belong
 * to anyone (redirecting stdout is the point), but its handle is only
 * reported back when it, too, was owner's: a service must never release
 * a handle another service issued.
 *
 * @param fd_table Process's file descriptor table
 * @param table_size Size of the file descriptor table
 * @param oldfd Descriptor to duplicate
 * @param newfd Descriptor slot to overwrite
 * @param owner Service asking for the copy
 * @param out_old_handle Receives oldfd's handle on success
 * @param out_replaced_handle Receives newfd's previous handle when owner
 * issued it, else 0
 * @return error_t OK on success, ERR_INVALID_FD when oldfd is unused or not
 * routed to owner, or newfd is out of range
 */
error_t dup_owned_fd(FileDescriptor* fd_table,
					 size_t table_size,
					 fd_t oldfd,
					 fd_t newfd,
					 ProcessId owner,
					 uint32_t* out_old_handle,
					 uint32_t* out_replaced_handle);

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
error_t copy_fd_table(FileDescriptor* dest,
					  const FileDescriptor* src,
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
void release_all_process_fds(FileDescriptor* fd_table, size_t table_size);

} // namespace kernel::fs
