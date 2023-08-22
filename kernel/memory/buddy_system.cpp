#include "buddy_system.hpp"

#include <algorithm>
#include <cmath>

#include "graphics/system_logger.hpp"

namespace
{
char memory_pool[sizeof(BuddySystem)];
}

BuddySystem* buddy_system;

BuddySystem::BuddySystem() {}

// Registers the given memory pages and address with the buddy system.
// Divides the memory into appropriate order blocks and adds them to the free lists.
void BuddySystem::RegisterMemory(int num_pages, void* addr)
{
	if (!addr || num_pages <= 0) {
		return;
	}

	while (num_pages > 0) {
		const auto order =
				std::min(static_cast<int>(std::log2(num_pages)), kMaxOrder);

		this->free_lists_[order].push_back(addr);

		auto next_addr = reinterpret_cast<uintptr_t>(addr) +
						 kUEFIPageSize * static_cast<uintptr_t>(std::pow(2, order));

		addr = reinterpret_cast<void*>(next_addr);
		num_pages -= static_cast<int>(std::pow(2, order));
	}
}

// Initializes the buddy system by iterating over the UEFI memory map.
// Registers available memory regions with the buddy system.
void InitializeBuddySystem(const MemoryMap& memory_map)
{
	buddy_system = new (memory_pool) BuddySystem();

	const auto memmap_base = reinterpret_cast<uintptr_t>(memory_map.buffer);
	const auto memmap_end = memmap_base + memory_map.map_size;

	for (uintptr_t iter = memmap_base; iter < memmap_end;
		 iter += memory_map.descriptor_size) {
		auto descriptor = reinterpret_cast<MemoryDescriptor*>(iter);

		if (!IsAvailableMemory(static_cast<MemoryType>(descriptor->type))) {
			continue;
		}

		buddy_system->RegisterMemory(
				static_cast<int>(descriptor->number_of_pages),
				reinterpret_cast<void*>(descriptor->physical_start));
	}
}
