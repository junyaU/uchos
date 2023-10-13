#pragma once

#include "../../UchLoaderPkg/memory_map.hpp"
#include "page.hpp"

#include <array>
#include <cstdint>

// 64 GiB
static const uint64_t MAX_PHYS_MEM_BYTES = 64UL * 1024 * 1024 * 1024;
const uint64_t TOTAL_PAGES = MAX_PHYS_MEM_BYTES / PAGE_SIZE;
const size_t BITMAP_ENTRY_SIZE = sizeof(unsigned long) * 8;

class bootstrap_allocator
{
public:
	bootstrap_allocator();

	void* allocate(size_t size);
	void free(void* addr, size_t size);

	void mark_available(void* addr, size_t size);

private:
	std::array<unsigned long, TOTAL_PAGES / BITMAP_ENTRY_SIZE> bitmap;
	void *memory_start, *memory_end;
};

extern bootstrap_allocator* boot_allocator;

void initialize_bootstrap_allocator(const MemoryMap& mem_map);