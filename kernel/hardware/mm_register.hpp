/**
 * @file hardware/mm_register.hpp
 * @brief Memory-mapped register access utilities
 *
 * This file provides template classes for safe and efficient access to
 * memory-mapped registers. Memory-mapped registers are used to interface
 * with hardware devices where registers are mapped to memory addresses,
 * allowing for direct manipulation through standard memory access methods.
 * 
 * The classes ensure proper volatile access and provide type-safe wrappers
 * around raw memory addresses.
 *
 * @date 2024
 */

#pragma once

#include <cstddef>
#include <cstdint>

/**
 * @brief Template metaprogramming utility to extract array length
 * @tparam T Type to examine
 * 
 * Base template - specialized below for arrays
 */
template<typename T>
struct ArrayLength {
};

/**
 * @brief Specialization for arrays to extract their length
 * @tparam T Element type of the array
 * @tparam N Size of the array
 * 
 * Provides compile-time access to array length through the 'value' member
 */
template<typename T, size_t N>
struct ArrayLength<T[N]> {
	static const size_t value = N; ///< The array length
};

/**
 * @brief Type-safe wrapper for memory-mapped registers
 * @tparam T Register type (must have a 'data' array member)
 * 
 * This class provides volatile-correct access to memory-mapped hardware
 * registers. It ensures all reads and writes are performed with volatile
 * semantics to prevent compiler optimizations that could break hardware
 * communication.
 */
template<typename T>
class MemoryMappedRegister
{
public:
	/**
	 * @brief Read the current value of the register
	 * 
	 * Performs a volatile read of all elements in the register's data array.
	 * This ensures the compiler doesn't optimize away the read.
	 * 
	 * @return Copy of the current register value
	 */
	T read() const
	{
		T tmp;
		for (size_t i = 0; i < len_; ++i) {
			tmp.data[i] = value_.data[i];
		}

		return tmp;
	}

	/**
	 * @brief Write a new value to the register
	 * 
	 * Performs a volatile write of all elements to the register's data array.
	 * This ensures the compiler doesn't optimize away or reorder the write.
	 * 
	 * @param value The value to write to the register
	 */
	void write(const T& value)
	{
		for (size_t i = 0; i < len_; ++i) {
			value_.data[i] = value.data[i];
		}
	}

private:
	volatile T value_; ///< The actual memory-mapped register (volatile)
	static const size_t len_ = ArrayLength<decltype(value_.data)>::value; ///< Length of the data array
};

/**
 * @brief Default bitmap structure for single-element register types
 * @tparam T The underlying data type
 * 
 * This structure provides a simple wrapper around a single data element,
 * making it compatible with the memory_mapped_register template which
 * expects types with a 'data' array member.
 */
template<typename T>
struct DefaultBitmap {
	T data[1]; ///< Single-element array to hold the value

	/**
	 * @brief Assignment operator
	 * @param value Value to assign
	 * @return Reference to this object
	 */
	DefaultBitmap& operator=(const T& value) { data[0] = value; return *this; }

	/**
	 * @brief Conversion operator to underlying type
	 * @return The stored value
	 */
	operator T() const { return data[0]; }
};

/**
 * @brief Wrapper class for memory-mapped arrays
 * @tparam T Element type of the array
 * 
 * This class provides a safe, STL-like interface to arrays that are
 * mapped at specific memory addresses. It's commonly used for accessing
 * arrays of registers or buffers in hardware devices.
 */
template<typename T>
class ArrayWrapper
{
public:
	using value_type = T; ///< Type of elements in the array
	using iterator = value_type*; ///< Iterator type
	using const_iterator = const value_type*; ///< Const iterator type

	/**
	 * @brief Construct an array wrapper at a specific memory address
	 * 
	 * @param addr The memory address where the array is located
	 * @param size Number of elements in the array
	 * 
	 * @warning The memory at addr must be valid and accessible
	 */
	ArrayWrapper(uintptr_t addr, size_t size)
		: array_{ reinterpret_cast<value_type*>(addr) }, size_{ size }
	{
	}

	/**
	 * @brief Get the number of elements in the array
	 * @return Array size
	 */
	size_t size() const { return size_; }
	
	/**
	 * @brief Get iterator to the beginning of the array
	 * @return Iterator to first element
	 */
	iterator begin() { return array_; }
	
	/**
	 * @brief Get iterator to the end of the array
	 * @return Iterator past the last element
	 */
	iterator end() { return array_ + size_; }
	
	/**
	 * @brief Get const iterator to the beginning of the array
	 * @return Const iterator to first element
	 */
	const_iterator cbegin() const { return array_; }
	
	/**
	 * @brief Get const iterator to the end of the array
	 * @return Const iterator past the last element
	 */
	const_iterator cend() const { return array_ + size_; }

	/**
	 * @brief Access array element by index
	 * @param index Element index
	 * @return Reference to the element
	 * @warning No bounds checking is performed
	 */
	value_type& operator[](size_t index) { return array_[index]; }

private:
	value_type* const array_; ///< Pointer to the memory-mapped array
	const size_t size_; ///< Number of elements in the array
};