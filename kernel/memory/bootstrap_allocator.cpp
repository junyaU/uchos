#include "bootstrap_allocator.hpp"
#include <limits>

bootstrap_allocator::bootstrap_allocator() : memory_start{ 0x0 }, memory_end{ 0x0 }
{
	bitmap.fill(1);
}

void bootstrap_allocator::mark_available(void* addr, size_t size)
{
	auto start = reinterpret_cast<uintptr_t>(addr) / PAGE_SIZE;
	auto end = (reinterpret_cast<uintptr_t>(addr) + size) / PAGE_SIZE;

	for (auto i = start; i < end; i++) {
		bitmap[i / 64] ^= 1UL << (i % 64);
	}

	if (memory_start == nullptr || memory_start > addr) {
		memory_start = addr;
	}

	memory_end = std::max(memory_end, reinterpret_cast<void*>(end * PAGE_SIZE));
}

char bootstrap_allocator_buffer[sizeof(bootstrap_allocator)];
bootstrap_allocator* boot_allocator;

void initialize_bootstrap_allocator(const MemoryMap& mem_map)
{
	boot_allocator = new (bootstrap_allocator_buffer) bootstrap_allocator();

	const auto mem_map_base = reinterpret_cast<uintptr_t>(mem_map.buffer);
	const auto mem_map_end = mem_map_base + mem_map.map_size;

	for (uintptr_t iter = mem_map_base; iter < mem_map_end;
		 iter += mem_map.descriptor_size) {
		auto desc = reinterpret_cast<MemoryDescriptor*>(iter);

		if (!IsAvailableMemory(static_cast<MemoryType>(desc->type))) {
			continue;
		}

		boot_allocator->mark_available(reinterpret_cast<void*>(desc->physical_start),
									   desc->number_of_pages * PAGE_SIZE);
	}
}