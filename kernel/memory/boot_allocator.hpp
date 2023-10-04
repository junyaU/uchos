#pragma once

#include <array>
#include <cstdint>

// 128 GiB
const int MAX_PHYS_MEM_BYTES = 128 * 1024 * 1024 * 1024;
const int TOTAL_PAGES = MAX_PHYS_MEM_BYTES / 4 * 1024;

class boot_allocator
{
public:
	boot_allocator();

	void* allocate(size_t size);
	void free(void* addr, size_t size);
	void register_memory(int num_pages, void* addr);

private:
	std::array<unsigned long, TOTAL_PAGES / sizeof(unsigned long) * 8> bitmap;
	void *memory_start, *memory_end;
};