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
	size_t size;
	size_t offset;
};

extern std::array<file_descriptor, 100> fds;