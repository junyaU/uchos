/**
 * @file hardware/mm_register.hpp
 *
 * @brief memory mapped register
 *
 * Provides functionality for reading and writing memory-mapped registers.
 * Memory-mapped registers are used to interface with hardware devices where
 * registers are mapped to memory addresses, allowing for direct manipulation
 * through standard memory access methods.
 *
 */

#pragma once

#include <cstddef>
#include <cstdint>

template<typename T>
struct array_length {
};

template<typename T, size_t N>
struct array_length<T[N]> {
	static const size_t value = N;
};

template<typename T>
class memory_mapped_register
{
public:
	T read() const
	{
		T tmp;
		for (size_t i = 0; i < len_; ++i) {
			tmp.data[i] = value_.data[i];
		}

		return tmp;
	}

	void write(const T& value)
	{
		for (size_t i = 0; i < len_; ++i) {
			value_.data[i] = value.data[i];
		}
	}

private:
	volatile T value_;
	static const size_t len_ = array_length<decltype(value_.data)>::value;
};

template<typename T>
struct default_bitmap {
	T data[1];

	default_bitmap& operator=(const T& value) { data[0] = value; }

	operator T() const { return data[0]; }
};

template<typename T>
class array_wrapper
{
public:
	using value_type = T;
	using iterator = value_type*;
	using const_iterator = const value_type*;

	array_wrapper(uintptr_t addr, size_t size)
		: array_{ reinterpret_cast<value_type*>(addr) }, size_{ size }
	{
	}

	size_t size() const { return size_; }
	iterator begin() { return array_; }
	iterator end() { return array_ + size_; }
	const_iterator cbegin() const { return array_; }
	const_iterator cend() const { return array_ + size_; }

	value_type& operator[](size_t index) { return array_[index]; }

private:
	value_type* const array_;
	const size_t size_;
};