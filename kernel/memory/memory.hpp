/*
 * @file memory/memory.hpp
 *
 * @brief Memory management namespace wrappers
 *
 * This header provides namespace-wrapped memory management functions
 * to help with the gradual migration to a fully namespaced kernel.
 */

#pragma once

#include <cstddef>

namespace kernel::memory {

// Forward declarations
void* allocate(size_t size, unsigned flags, int align = 1);
void deallocate(void* addr);

// Convenience functions that match the old API
inline void* kmalloc(size_t size, unsigned flags, int align = 1)
{
	return allocate(size, flags, align);
}

inline void kfree(void* addr)
{
	deallocate(addr);
}

} // namespace kernel::memory