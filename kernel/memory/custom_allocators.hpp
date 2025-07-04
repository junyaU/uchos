/**
 * @file memory/custom_allocators.hpp
 * @brief Custom allocator implementations for kernel use
 * 
 * This file provides custom allocator implementations that can be used with
 * STL containers in the kernel environment. These allocators use the kernel's
 * memory management system instead of standard malloc/free.
 * 
 * @date 2024
 */

#pragma once

#include "memory/slab.hpp"
#include <cstddef>
#include <limits>

namespace kernel::memory {

/**
 * @brief Fixed-size pool allocator for efficient allocation of small objects
 * @tparam T Type of objects to allocate
 * @tparam pool_size Number of objects that can be stored in the pool
 * 
 * This allocator maintains a fixed-size pool of objects and uses a free list
 * for fast allocation and deallocation. It's ideal for frequently allocated/
 * deallocated objects of the same type.
 */
template<typename T, std::size_t pool_size>
class PoolAllocator
{
public:
	using value_type = T;

	/**
	 * @brief Construct a pool allocator and initialize the free list
	 */
	PoolAllocator() noexcept
	{
		for (std::size_t i = 0; i < pool_size; i++) {
			*reinterpret_cast<T**>(&pool_[i * sizeof(T)]) =
					reinterpret_cast<T*>(&pool_[(i + 1) * sizeof(T)]);
		}

		*reinterpret_cast<T**>(&pool_[(pool_size - 1) * sizeof(T)]) = nullptr;
		next_free_ = reinterpret_cast<T*>(&pool_[0]);
	};

	/**
	 * @brief Copy constructor for allocator rebinding
	 * @tparam U Type of the source allocator
	 */
	template<typename U>
	PoolAllocator(const PoolAllocator<U, pool_size>&) noexcept
	{
	}

	/**
	 * @brief Rebind allocator to another type
	 * @tparam U New type to allocate
	 */
	template<typename U>
	struct rebind {
		using other = PoolAllocator<U, pool_size>;
	};

	/**
	 * @brief Allocate a single object from the pool
	 * @param n Number of objects to allocate (must be 1)
	 * @return Pointer to allocated object, or nullptr if pool is exhausted
	 * @note This allocator only supports single object allocation (n must be 1)
	 */
	T* allocate(std::size_t n)
	{
		if (n != 1 || next_free_ == nullptr) {
			return nullptr;
		}

		T* result = next_free_;
		next_free_ = *reinterpret_cast<T**>(next_free_);
		return result;
	}

	/**
	 * @brief Return an object to the pool
	 * @param ptr Pointer to object to deallocate
	 * @param n Number of objects (ignored)
	 */
	void deallocate(T* ptr, std::size_t)
	{
		*reinterpret_cast<T**>(ptr) = next_free_;
		next_free_ = ptr;
	}

private:
	alignas(T) unsigned char pool_[sizeof(T) * pool_size]; /**< Fixed-size pool storage */
	T* next_free_; /**< Pointer to next free object in the pool */
};

/**
 * @brief General-purpose kernel allocator that uses alloc/free
 * @tparam T Type of objects to allocate
 * 
 * This allocator provides a standard allocator interface for STL containers
 * but uses the kernel's alloc/free functions instead of standard malloc/free.
 * It's suitable for dynamic allocations of varying sizes.
 */
template<typename T>
class kernel_allocator
{
public:
	using value_type = T;
	using pointer = T*;
	using const_pointer = const T*;
	using reference = T&;
	using const_reference = const T&;
	using size_type = std::size_t;
	using difference_type = std::ptrdiff_t;

	/**
	 * @brief Default constructor
	 */
	kernel_allocator() noexcept {}
	
	/**
	 * @brief Copy constructor for allocator rebinding
	 * @tparam U Type of the source allocator
	 */
	template<typename U>
	kernel_allocator(const kernel_allocator<U>&) noexcept
	{
	}

	/**
	 * @brief Allocate memory for n objects
	 * @param n Number of objects to allocate
	 * @return Pointer to allocated memory, or nullptr on failure
	 * @note Memory is zero-initialized
	 */
	T* allocate(std::size_t n) noexcept
	{
		if (n > std::numeric_limits<std::size_t>::max() / sizeof(T)) {
			return nullptr; // Overflow check
		}
		return static_cast<T*>(alloc(n * sizeof(T), ALLOC_ZEROED));
	}

	/**
	 * @brief Deallocate memory
	 * @param p Pointer to memory to deallocate
	 * @param n Number of objects (ignored)
	 */
	void deallocate(T* p, std::size_t) noexcept { free(p); }
};

/**
 * @brief Equality comparison for kernel allocators
 * @return Always true (all kernel_allocator instances are equal)
 */
template<typename T, typename U>
bool operator==(const kernel_allocator<T>&, const kernel_allocator<U>&)
{
	return true;
}

/**
 * @brief Inequality comparison for kernel allocators
 * @return Always false (all kernel_allocator instances are equal)
 */
template<typename T, typename U>
bool operator!=(const kernel_allocator<T>&, const kernel_allocator<U>&)
{
	return false;
}

} // namespace kernel::memory