#include "buddy_system.hpp"

namespace {
char memory_pool[sizeof(BuddySystem)];
}

BuddySystem* buddy_system;

BuddySystem::BuddySystem() {}

void InitializeBuddySystem(const MemoryMap& memory_map) {
    buddy_system = new (memory_pool) BuddySystem();

    const auto memmap_base = reinterpret_cast<uintptr_t>(memory_map.buffer);
    const auto memmap_end = memmap_base + memory_map.map_size;

    for (uintptr_t iter = memmap_base; iter < memmap_end;
         iter += memory_map.descriptor_size) {
        auto descriptor = reinterpret_cast<MemoryDescriptor*>(iter);

        if (!IsAvailableMemory(static_cast<MemoryType>(descriptor->type))) {
            continue;
        }
    }
}
