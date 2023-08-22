#pragma once

#include <cstddef>
#include <new>

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