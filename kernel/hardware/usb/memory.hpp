#pragma once

#include <cstddef>

namespace usb
{
static const size_t MEMORY_POOL_SIZE = 4096UL * 32;

void* alloc_memory(size_t size, unsigned int alignment, unsigned int boundary);

template<class T>
T* alloc_array(size_t num_objs, unsigned int alignment, unsigned int boundary)
{
	return reinterpret_cast<T*>(
			alloc_memory(sizeof(T) * num_objs, alignment, boundary));
}

void free_memory(void* p);

template<class T, unsigned int alignment = 64, unsigned int boundary = 4096>
class allocator
{
public:
	using size_type = size_t;
	using pointer = T*;
	using value_type = T;

	allocator() noexcept = default;
	allocator(const allocator&) noexcept = default;
	template<class U>
	allocator(const allocator<U>&) noexcept
	{
	}
	~allocator() noexcept = default;
	allocator& operator=(const allocator&) = default;

	pointer allocate(size_type n) { return alloc_array<T>(n, alignment, boundary); }

	void deallocate(pointer p, size_type n) { free_memory(p); }
};

} // namespace usb
