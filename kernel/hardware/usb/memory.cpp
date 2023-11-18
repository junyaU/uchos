#include "memory.hpp"

#include <cstdint>

namespace
{
template<class T>
T ceil(T value, unsigned int alignment)
{
	return (value + alignment - 1) & ~static_cast<T>(alignment - 1);
}

template<class T, class U>
T mask_bits(T value, U mask)
{
	return value & ~static_cast<T>(mask - 1);
}

} // namespace

namespace usb
{
alignas(64) uint8_t memory_pool[MEMORY_POOL_SIZE];
uintptr_t memory_pool_ptr = reinterpret_cast<uintptr_t>(memory_pool);

void* alloc_memory(size_t size, unsigned int alignment, unsigned int boundary)
{
	if (alignment > 0) {
		memory_pool_ptr = ceil(memory_pool_ptr, alignment);
	}

	if (boundary > 0) {
		auto next_boundary = ceil(memory_pool_ptr, boundary);
		if (next_boundary < memory_pool_ptr + size) {
			memory_pool_ptr = next_boundary;
		}
	}

	if (reinterpret_cast<uintptr_t>(memory_pool) + MEMORY_POOL_SIZE <
		memory_pool_ptr + size) {
		return nullptr;
	}

	auto p = memory_pool_ptr;
	memory_pool_ptr += size;
	return reinterpret_cast<void*>(p);
}

void free_memory(void* p) {}

} // namespace usb
