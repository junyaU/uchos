#include "fs/file_descriptor.hpp"
#include "graphics/log.hpp"
#include <array>
#include <cstddef>
#include <cstring>
#include <libs/common/process_id.hpp>
#include <libs/common/types.hpp>

namespace kernel::fs
{

std::array<file_descriptor, MAX_FILE_DESCRIPTORS> fds;

file_descriptor* get_fd(int fd)
{
	if (fd < 0 || fd >= MAX_FILE_DESCRIPTORS) {
		return nullptr;
	}

	return &fds[fd];
}

file_descriptor* register_fd(const char* name, size_t size, ProcessId pid)
{
	for (size_t i = 0; i < MAX_FILE_DESCRIPTORS; ++i) {
		if (fds[i].name[0] == '\0') {
			fds[i].size = size;
			fds[i].offset = 0;
			strncpy(fds[i].name, name, 11);
			return &fds[i];
		}
	}

	return nullptr;
}

void init_fds()
{
	fds = std::array<file_descriptor, MAX_FILE_DESCRIPTORS>();

	for (size_t i = 0; i < MAX_FILE_DESCRIPTORS; ++i) {
		memset(&fds[i], 0, sizeof(file_descriptor));
	}
}

// Process-local file descriptor management implementation

void init_process_fd_table(file_descriptor_entry* fd_table, size_t table_size)
{
	if (fd_table == nullptr) {
		return;
	}

	// Initialize all entries as unused
	for (size_t i = 0; i < table_size; ++i) {
		fd_table[i].in_use = false;
		memset(&fd_table[i].fd, 0, sizeof(file_descriptor));
		fd_table[i].redirect_to = NO_FD;
	}

	// Set up standard file descriptors
	// stdin
	if (STDIN_FILENO < table_size) {
		fd_table[STDIN_FILENO].in_use = true;
		strncpy(fd_table[STDIN_FILENO].fd.name, "stdin", 11);
		fd_table[STDIN_FILENO].fd.size = 0;
		fd_table[STDIN_FILENO].fd.offset = 0;
		fd_table[STDIN_FILENO].redirect_to = NO_FD;
	}

	// stdout
	if (STDOUT_FILENO < table_size) {
		fd_table[STDOUT_FILENO].in_use = true;
		strncpy(fd_table[STDOUT_FILENO].fd.name, "stdout", 11);
		fd_table[STDOUT_FILENO].fd.size = 0;
		fd_table[STDOUT_FILENO].fd.offset = 0;
		fd_table[STDOUT_FILENO].redirect_to = NO_FD;
	}

	// stderr
	if (STDERR_FILENO < table_size) {
		fd_table[STDERR_FILENO].in_use = true;
		strncpy(fd_table[STDERR_FILENO].fd.name, "stderr", 11);
		fd_table[STDERR_FILENO].fd.size = 0;
		fd_table[STDERR_FILENO].fd.offset = 0;
		fd_table[STDERR_FILENO].redirect_to = NO_FD;
	}
}

fd_t allocate_process_fd(file_descriptor_entry* fd_table,
                         size_t table_size,
                         const char* name,
                         size_t size,
                         ProcessId pid)
{
	if (fd_table == nullptr || name == nullptr) {
		return NO_FD;
	}

	// Find first unused entry
	for (size_t i = 0; i < table_size; ++i) {
		if (!fd_table[i].in_use) {
			fd_table[i].in_use = true;
			// fd number is the index itself
			fd_table[i].fd.size = size;
			fd_table[i].fd.offset = 0;
			strncpy(fd_table[i].fd.name, name, 10);
			fd_table[i].fd.name[10] = '\0';
			fd_table[i].redirect_to = NO_FD;
			return i;
		}
	}

	LOG_ERROR("Process %d: No available file descriptors", pid.raw());
	return NO_FD;
}

file_descriptor_entry* get_process_fd(file_descriptor_entry* fd_table, size_t table_size, fd_t fd)
{
	if (fd_table == nullptr || fd < 0 || fd >= table_size) {
		return nullptr;
	}

	if (!fd_table[fd].in_use) {
		return nullptr;
	}

	// Handle redirection
	if (fd_table[fd].redirect_to != NO_FD) {
		return get_process_fd(fd_table, table_size, fd_table[fd].redirect_to);
	}

	return &fd_table[fd];
}

error_t release_process_fd(file_descriptor_entry* fd_table, size_t table_size, fd_t fd)
{
	if (fd_table == nullptr || fd < 0 || fd >= table_size) {
		return ERR_INVALID_ARG;
	}

	// Don't release standard descriptors
	if (fd == STDIN_FILENO || fd == STDOUT_FILENO || fd == STDERR_FILENO) {
		return ERR_INVALID_FD;
	}

	if (!fd_table[fd].in_use) {
		return ERR_INVALID_FD;
	}

	// Clear the entry
	fd_table[fd].in_use = false;
	fd_table[fd].redirect_to = NO_FD;
	memset(fd_table[fd].fd.name, 0, sizeof(fd_table[fd].fd.name));

	return OK;
}

error_t copy_fd_table(file_descriptor_entry* dest,
                      const file_descriptor_entry* src,
                      size_t table_size,
                      ProcessId child_pid)
{
	if (dest == nullptr || src == nullptr) {
		return ERR_INVALID_ARG;
	}

	// Copy each entry
	for (size_t i = 0; i < table_size; ++i) {
		if (src[i].in_use) {
			dest[i] = src[i];
			// No need to update PID as it's managed per-process now
		} else {
			dest[i].in_use = false;
			dest[i].redirect_to = NO_FD;
		}
	}

	return OK;
}

void release_all_process_fds(file_descriptor_entry* fd_table, size_t table_size)
{
	if (fd_table == nullptr) {
		return;
	}

	// Release all non-standard file descriptors
	for (size_t i = 0; i < table_size; ++i) {
		if (i != STDIN_FILENO && i != STDOUT_FILENO && i != STDERR_FILENO) {
			if (fd_table[i].in_use) {
				release_process_fd(fd_table, table_size, i);
			}
		}
	}
}

}  // namespace kernel::fs
