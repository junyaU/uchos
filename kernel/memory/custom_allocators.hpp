#pragma once

#include "memory/slab.hpp"
#include <cstddef>
#include <limits>

template<typename T, std::size_t pool_size>
class PoolAllocator
{
public:
	using value_type = T;

	PoolAllocator() noexcept
	{
		for (std::size_t i = 0; i < pool_size; i++) {
			*reinterpret_cast<T**>(&pool_[i * sizeof(T)]) =
					reinterpret_cast<T*>(&pool_[(i + 1) * sizeof(T)]);
		}

		*reinterpret_cast<T**>(&pool_[(pool_size - 1) * sizeof(T)]) = nullptr;
		next_free_ = reinterpret_cast<T*>(&pool_[0]);
	};

	template<typename U>
	PoolAllocator(const PoolAllocator<U, pool_size>&) noexcept
	{
	}

	template<typename U>
	struct rebind {
		using other = PoolAllocator<U, pool_size>;
	};

	T* allocate(std::size_t n)
	{
		if (n != 1 || next_free_ == nullptr) {
			return nullptr;
		}

		T* result = next_free_;
		next_free_ = *reinterpret_cast<T**>(next_free_);
		return result;
	}

	void deallocate(T* ptr, std::size_t)
	{
		*reinterpret_cast<T**>(ptr) = next_free_;
		next_free_ = ptr;
	}

private:
	alignas(T) unsigned char pool_[sizeof(T) * pool_size];
	T* next_free_;
};

#include <cstddef>
#include <limits>

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

	kernel_allocator() noexcept {}
	template<typename U>
	kernel_allocator(const kernel_allocator<U>&) noexcept
	{
	}

	T* allocate(std::size_t n) noexcept
	{
		if (n > std::numeric_limits<std::size_t>::max() / sizeof(T)) {
			return nullptr; // オーバーフローチェック
		}
		return static_cast<T*>(kmalloc(n * sizeof(T), KMALLOC_ZEROED));
	}

	void deallocate(T* p, std::size_t) noexcept { kfree(p); }
};

template<typename T, typename U>
bool operator==(const kernel_allocator<T>&, const kernel_allocator<U>&)
{
	return true;
}

template<typename T, typename U>
bool operator!=(const kernel_allocator<T>&, const kernel_allocator<U>&)
{
	return false;
}