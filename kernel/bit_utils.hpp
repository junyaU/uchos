/**
 * @file bit_utils.hpp
 * @brief Bit manipulation and alignment utility functions
 * 
 * This file provides utility functions for bit manipulation operations
 * and memory alignment calculations commonly used throughout the kernel.
 * These functions are essential for memory management, data structure
 * alignment, and efficient bit-level operations.
 * 
 * @date 2024
 */

#pragma once

/**
 * @brief Calculate the number of bits required to represent a value
 * 
 * Computes the minimum number of bits needed to store the given value.
 * For example, bit_width(5) returns 3 because 5 requires 3 bits (101 in binary).
 * 
 * @param x The value to analyze
 * @return The number of bits required to represent x
 * 
 * @note Returns 0 for x = 0
 * @note This is equivalent to floor(log2(x)) + 1 for x > 0
 * 
 * @example
 * bit_width(0) = 0
 * bit_width(1) = 1
 * bit_width(4) = 3
 * bit_width(5) = 3
 * bit_width(8) = 4
 */
unsigned int bit_width(unsigned int x);

/**
 * @brief Calculate the ceiling of bit width for a value
 * 
 * Computes the number of bits required to represent values from 0 to x-1.
 * This is useful for determining the size of bit fields or indices.
 * 
 * @param x The upper bound value (exclusive)
 * @return The number of bits required to represent values from 0 to x-1
 * 
 * @note Returns 0 for x = 0
 * @note This is equivalent to ceil(log2(x)) for x > 0
 * 
 * @example
 * bit_width_ceil(0) = 0
 * bit_width_ceil(1) = 0
 * bit_width_ceil(2) = 1
 * bit_width_ceil(5) = 3
 * bit_width_ceil(8) = 3
 * bit_width_ceil(9) = 4
 */
unsigned int bit_width_ceil(unsigned int x);

/**
 * @brief Align a value up to the nearest multiple of alignment
 * 
 * Rounds up the given value to the next multiple of the specified alignment.
 * This is commonly used for memory address alignment and size calculations.
 * 
 * @param value The value to align
 * @param alignment The alignment boundary (must be a power of 2)
 * @return The aligned value
 * 
 * @note The alignment must be a power of 2 for correct operation
 * @note If value is already aligned, it is returned unchanged
 * 
 * @example
 * align_up(10, 4) = 12
 * align_up(16, 4) = 16
 * align_up(17, 8) = 24
 */
unsigned int align_up(unsigned int value, unsigned int alignment);