/*
 * @file memory/memory.cpp
 *
 * @brief Memory management namespace wrapper implementations
 */

#include "memory.hpp"
#include "slab.hpp"

namespace kernel::memory {

void* allocate(size_t size, unsigned flags, int align)
{
	return ::kmalloc(size, flags, align);
}

void deallocate(void* addr)
{
	::kfree(addr);
}

} // namespace kernel::memory