#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <list>

#include "../../UchLoaderPkg/memory_map.hpp"
#include "pool.hpp"

static const size_t kPoolSize = 4 * 1024;

static const auto kMaxOrder = 8;

class BuddySystem {
   public:
    BuddySystem();

   private:
    std::array<std::list<void *, PoolAllocator<void *, kPoolSize>>, kMaxOrder>
        free_lists_;
};

extern BuddySystem *buddy_system;

void InitializeBuddySystem(const MemoryMap &memory_map);