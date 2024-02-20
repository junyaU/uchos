/*
 * @file file_system/file_descriptor.hpp
 *
 */
#pragma once

#include <cstddef>

struct file_descriptor {
	virtual ~file_descriptor() = default;
	virtual size_t read(void* buf, size_t len) = 0;
	virtual size_t write(const void* buf, size_t len) = 0;
};
