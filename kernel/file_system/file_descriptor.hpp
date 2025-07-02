/*
 * @file file_system/file_descriptor.hpp
 *
 */
#pragma once

#include <array>
#include <cstddef>
#include <libs/common/types.hpp>
#include <libs/common/process_id.hpp>

namespace kernel::fs
{

struct file_descriptor {
	fd_t fd;
	char name[11];
	ProcessId pid;
	size_t size;
	size_t offset;
};

constexpr size_t MAX_FILE_DESCRIPTORS = 100;
extern std::array<file_descriptor, MAX_FILE_DESCRIPTORS> fds;

file_descriptor* get_fd(int fd);

file_descriptor* register_fd(const char* name, size_t size, ProcessId pid);

void init_fds();

} // namespace kernel::fs
