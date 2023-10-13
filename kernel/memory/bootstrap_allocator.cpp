#include "bootstrap_allocator.hpp"
#include <limits>

#include "graphics/system_logger.hpp"

bootstrap_allocator::bootstrap_allocator() : memory_start{ 0x0 }, memory_end{ 0x0 }
{
	bitmap.fill(1);
}

void* bootstrap_allocator::allocate(size_t size)
{
	const size_t num_pages = (size + PAGE_SIZE - 1) / PAGE_SIZE;

	system_logger->Printf("num_pages: %d\n", num_pages);

	const auto memory_start_index =
			reinterpret_cast<uintptr_t>(memory_start) / PAGE_SIZE;
	const auto memory_end_index =
			reinterpret_cast<uintptr_t>(memory_end) / PAGE_SIZE;

	size_t consecutive_start_index = 0;
	size_t num_consecutive_pages = 0;

	for (size_t i = memory_start_index; i < memory_end_index; i++) {
		if (bitmap[i / BITMAP_ENTRY_SIZE] & (1UL << (i % BITMAP_ENTRY_SIZE))) {
			num_consecutive_pages = 0;
			continue;
		}

		if (num_consecutive_pages == 0) {
			consecutive_start_index = i;
		}

		num_consecutive_pages++;

		if (num_consecutive_pages == num_pages) {
			for (size_t j = consecutive_start_index;
				 j < consecutive_start_index + num_pages; j++) {
				bitmap[j / BITMAP_ENTRY_SIZE] |= 1UL << (j % BITMAP_ENTRY_SIZE);
			}

			return reinterpret_cast<void*>(consecutive_start_index * PAGE_SIZE);
		}
	}

	return nullptr;
}

void free(void* addr, size_t size) {}

void bootstrap_allocator::mark_available(void* addr, size_t size)
{
	auto start = reinterpret_cast<uintptr_t>(addr) / PAGE_SIZE;
	auto end = (reinterpret_cast<uintptr_t>(addr) + size) / PAGE_SIZE;

	for (auto i = start; i < end; i++) {
		bitmap[i / BITMAP_ENTRY_SIZE] &= ~(1UL << (i % BITMAP_ENTRY_SIZE));
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