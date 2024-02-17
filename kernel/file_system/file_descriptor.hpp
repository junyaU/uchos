/*
 * @file file_system/file_descriptor.hpp
 *
 */
#pragma once

#include "fat.hpp"
#include <cstddef>

namespace file_system
{
struct file_descriptor {
	directory_entry& entry;
	unsigned long current_cluster;
	size_t current_cluster_offset;
	size_t current_file_offset;

	file_descriptor(directory_entry& e)
		: entry(e),
		  current_cluster(0),
		  current_cluster_offset(0),
		  current_file_offset(0)
	{
	}

	size_t read(void* buf, size_t len);
};

} // namespace file_system
