#pragma once

struct MemoryMap;

#include <stddef.h>
#include <array>
#include <cstdint>
#include "page.hpp"

namespace kernel::memory
{

// 64 GiB
static const uint64_t MAX_PHYS_MEM_BYTES = 64UL * 1024 * 1024 * 1024;
const uint64_t TOTAL_PAGES = MAX_PHYS_MEM_BYTES / PAGE_SIZE;
const size_t BITMAP_ENTRY_SIZE = sizeof(unsigned long) * 8;

class BootstrapAllocator
{
public:
	BootstrapAllocator();

	void* allocate(size_t size);
	void free(void* addr, size_t size);
	void mark_available(void* addr, size_t size);

	void show_available_memory() const;

	bool is_bit_set(size_t i) const
	{
		return (bitmap_[i / BITMAP_ENTRY_SIZE] & (1UL << (i % BITMAP_ENTRY_SIZE))) !=
			   0U;
	}

	size_t start_index() const
	{
		return reinterpret_cast<uintptr_t>(memory_start_) / PAGE_SIZE;
	}

	size_t end_index() const
	{
		return reinterpret_cast<uintptr_t>(memory_end_) / PAGE_SIZE;
	}

private:
	std::array<unsigned long, TOTAL_PAGES / BITMAP_ENTRY_SIZE> bitmap_;
	void *memory_start_, *memory_end_;
};

extern BootstrapAllocator* boot_allocator;

void initialize(const MemoryMap& mem_map);
void initialize_heap();
void disable();

} // namespace kernel::memory