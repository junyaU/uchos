#include "bootstrap_allocator.hpp"
#include <climits>
#include <limits>
#include <sys/types.h>

#include "graphics/system_logger.hpp"

bootstrap_allocator::bootstrap_allocator()
	: bitmap_{}, memory_start_{ 0x0 }, memory_end_{ 0x0 }
{
	bitmap_.fill(ULONG_MAX);
}

void* bootstrap_allocator::allocate(size_t size)
{
	const size_t num_pages = (size + PAGE_SIZE - 1) / PAGE_SIZE;

	size_t consecutive_start_index = 0;
	size_t num_consecutive_pages = 0;

	for (size_t i = start_index(); i < end_index(); i++) {
		if (is_bit_set(i)) {
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
				bitmap_[j / BITMAP_ENTRY_SIZE] |= 1UL << (j % BITMAP_ENTRY_SIZE);
			}

			return reinterpret_cast<void*>(consecutive_start_index * PAGE_SIZE);
		}
	}

	system_logger->Printf("failed to allocate %u bytes\n", size);

	return nullptr;
}

void bootstrap_allocator::free(void* addr, size_t size)
{
	auto start = reinterpret_cast<uintptr_t>(addr) / PAGE_SIZE;
	auto end = (reinterpret_cast<uintptr_t>(addr) + size) / PAGE_SIZE;

	for (auto i = start; i < end; i++) {
		bitmap_[i / BITMAP_ENTRY_SIZE] &= ~(1UL << (i % BITMAP_ENTRY_SIZE));
	}
}

void bootstrap_allocator::mark_available(void* addr, size_t size)
{
	auto start = reinterpret_cast<uintptr_t>(addr) / PAGE_SIZE;
	auto end = (reinterpret_cast<uintptr_t>(addr) + size) / PAGE_SIZE;

	for (auto i = start; i < end; i++) {
		bitmap_[i / BITMAP_ENTRY_SIZE] &= ~(1UL << (i % BITMAP_ENTRY_SIZE));
	}

	memory_end_ = std::max(memory_end_, reinterpret_cast<void*>(end * PAGE_SIZE));
}

void bootstrap_allocator::show_available_memory() const
{
	size_t available_pages = 0;

	for (size_t i = start_index(); i < end_index(); i++) {
		if (!is_bit_set(i)) {
			available_pages++;
		}
	}

	system_logger->Printf("available memory: %u MiB / %u MiB\n",
						  available_pages * PAGE_SIZE / 1024 / 1024,
						  (end_index() - start_index()) * PAGE_SIZE / 1024 / 1024);
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

extern "C" caddr_t program_break, program_break_end;

void initialize_heap()
{
	// 128 MiB
	const int heap_pages = 64 * 512;
	const size_t heap_size = heap_pages * PAGE_SIZE;

	auto heap = boot_allocator->allocate(heap_size);
	if (heap == nullptr) {
		system_logger->Printf("failed to allocate heap\n");
		return;
	}

	program_break = reinterpret_cast<caddr_t>(heap);
	program_break_end = program_break + heap_size;
}