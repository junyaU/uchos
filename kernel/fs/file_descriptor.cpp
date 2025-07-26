#include "fs/file_descriptor.hpp"
#include "graphics/log.hpp"
#include <cstddef>
#include <cstring>
#include <libs/common/process_id.hpp>
#include <libs/common/types.hpp>

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

	// Set up standard file descriptors
	// stdin
	if (STDIN_FILENO < table_size) {
		strncpy(fd_table[STDIN_FILENO].name, "stdin", 11);
		fd_table[STDIN_FILENO].size = 0;
		fd_table[STDIN_FILENO].offset = 0;
	}

	// stdout
	if (STDOUT_FILENO < table_size) {
		strncpy(fd_table[STDOUT_FILENO].name, "stdout", 11);
		fd_table[STDOUT_FILENO].size = 0;
		fd_table[STDOUT_FILENO].offset = 0;
	}

	// stderr
	if (STDERR_FILENO < table_size) {
		strncpy(fd_table[STDERR_FILENO].name, "stderr", 11);
		fd_table[STDERR_FILENO].size = 0;
		fd_table[STDERR_FILENO].offset = 0;
	}
}

fd_t allocate_process_fd(FileDescriptor* fd_table,
                         size_t table_size,
                         const char* name,
                         size_t size,
                         ProcessId pid)
{
	if (fd_table == nullptr || name == nullptr) {
		LOG_ERROR("Invalid arguments for allocate_process_fd");
		return NO_FD;
	}

	// Find first unused entry
	for (size_t i = 0; i < table_size; ++i) {
		if (fd_table[i].is_unused()) {
			fd_table[i].size = size;
			fd_table[i].offset = 0;
			strncpy(fd_table[i].name, name, 10);
			fd_table[i].name[10] = '\0';
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
		LOG_ERROR("fd: %d", fd);
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

}  // namespace kernel::fs
