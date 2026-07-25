#include "fs/file_descriptor.hpp"
#include <cstddef>
#include <cstring>
#include <libs/common/process_id.hpp>
#include <libs/common/types.hpp>
#include "log/log.hpp"

namespace kernel::fs
{

void init_process_fd_table(FileDescriptor* fd_table, size_t table_size)
{
	if (fd_table == nullptr) {
		return;
	}

	// Initialize all entries as unused
	for (size_t i = 0; i < table_size; ++i) {
		fd_table[i].clear();
	}

	// Set up standard file descriptors. The console routing is decided
	// HERE, at entry creation — the syscall layer just follows it (issue
	// #315). Console fds have no server-side object, so handle stays 0.
	// This init moves to the user-side runtime in 3b.
	// stdin
	if (STDIN_FILENO < table_size) {
		strncpy(fd_table[STDIN_FILENO].name, "stdin",
				sizeof(fd_table[STDIN_FILENO].name) - 1);
		fd_table[STDIN_FILENO].route = FdRoute::CONSOLE;
		fd_table[STDIN_FILENO].dest = process_ids::SHELL;
	}

	// stdout
	if (STDOUT_FILENO < table_size) {
		strncpy(fd_table[STDOUT_FILENO].name, "stdout",
				sizeof(fd_table[STDOUT_FILENO].name) - 1);
		fd_table[STDOUT_FILENO].route = FdRoute::CONSOLE;
		fd_table[STDOUT_FILENO].dest = process_ids::SHELL;
	}

	// stderr
	if (STDERR_FILENO < table_size) {
		strncpy(fd_table[STDERR_FILENO].name, "stderr",
				sizeof(fd_table[STDERR_FILENO].name) - 1);
		fd_table[STDERR_FILENO].route = FdRoute::CONSOLE;
		fd_table[STDERR_FILENO].dest = process_ids::SHELL;
	}
}

fd_t allocate_process_fd(FileDescriptor* fd_table,
						 size_t table_size,
						 const char* name,
						 uint32_t handle,
						 ProcessId pid)
{
	if (fd_table == nullptr || name == nullptr) {
		LOG_ERROR("Invalid arguments for allocate_process_fd");
		return NO_FD;
	}

	// Find first unused entry
	for (size_t i = 0; i < table_size; ++i) {
		if (fd_table[i].is_unused()) {
			// This helper is FS-side code (it moves into the FS server in
			// 3b), so entries it creates route to the FAT32 service; the
			// syscall layer only ever reads this (issue #315). All file
			// state lives behind the handle in the FS ledger.
			fd_table[i].route = FdRoute::FS;
			fd_table[i].dest = process_ids::FS_FAT32;
			fd_table[i].handle = handle;
			strncpy(fd_table[i].name, name, sizeof(fd_table[i].name) - 1);
			fd_table[i].name[sizeof(fd_table[i].name) - 1] = '\0';
			return i;
		}
	}

	LOG_ERROR("Process %d: No available file descriptors", pid.raw());
	return NO_FD;
}

FileDescriptor* get_process_fd(FileDescriptor* fd_table, size_t table_size, fd_t fd)
{
	if (fd_table == nullptr || fd < 0 || fd >= table_size) {
		return nullptr;
	}

	if (fd_table[fd].is_unused()) {
		LOG_ERROR("fd %d is not in use", fd);
		return nullptr;
	}

	return &fd_table[fd];
}

error_t release_process_fd(FileDescriptor* fd_table, size_t table_size, fd_t fd)
{
	if (fd_table == nullptr || fd < 0 || fd >= table_size) {
		return ERR_INVALID_ARG;
	}

	// Don't release standard descriptors
	if (fd == STDIN_FILENO || fd == STDOUT_FILENO || fd == STDERR_FILENO) {
		return ERR_INVALID_FD;
	}

	if (fd_table[fd].is_unused()) {
		return ERR_INVALID_FD;
	}

	// Clear the entry
	fd_table[fd].clear();

	return OK;
}

error_t dup_process_fd(FileDescriptor* fd_table,
					   size_t table_size,
					   fd_t oldfd,
					   fd_t newfd)
{
	FileDescriptor* old_entry = get_process_fd(fd_table, table_size, oldfd);
	if (old_entry == nullptr) {
		return ERR_INVALID_FD;
	}

	if (newfd < 0 || newfd >= table_size) {
		return ERR_INVALID_FD;
	}

	fd_table[newfd] = *old_entry;

	return OK;
}

error_t copy_fd_table(FileDescriptor* dest,
					  const FileDescriptor* src,
					  size_t table_size,
					  ProcessId child_pid)
{
	if (dest == nullptr || src == nullptr) {
		return ERR_INVALID_ARG;
	}

	// Copy each entry
	for (size_t i = 0; i < table_size; ++i) {
		if (src[i].is_used()) {
			dest[i] = src[i];
			// No need to update PID as it's managed per-process now
		} else {
			dest[i].clear();
		}
	}

	return OK;
}

void release_all_process_fds(FileDescriptor* fd_table, size_t table_size)
{
	if (fd_table == nullptr) {
		return;
	}

	// Release all non-standard file descriptors
	for (size_t i = 0; i < table_size; ++i) {
		if (i != STDIN_FILENO && i != STDOUT_FILENO && i != STDERR_FILENO) {
			if (fd_table[i].is_used()) {
				release_process_fd(fd_table, table_size, i);
			}
		}
	}
}

} // namespace kernel::fs
