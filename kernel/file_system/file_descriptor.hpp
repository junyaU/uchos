/*
 * @file file_system/file_descriptor.hpp
 *
 */
#pragma once

#include <array>
#include <cstddef>
#include <libs/common/types.hpp>

struct file_descriptor {
	fd_t fd;
	char name[11];
	pid_t pid;
	size_t size;
	size_t offset;
};

constexpr size_t MAX_FILE_DESCRIPTORS = 100;
extern std::array<file_descriptor, MAX_FILE_DESCRIPTORS> fds;

file_descriptor* register_fd(const char* name, size_t size, pid_t pid);

void init_fds();
