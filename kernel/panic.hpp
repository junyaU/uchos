/**
 * @file panic.hpp
 * @brief Kernel panic and assertion facilities
 *
 * This file provides panic functionality for fatal kernel error handling
 * and assertion features for debugging and validation.
 *
 * @date 2024
 */

#pragma once

namespace kernel
{

/**
 * @brief Trigger a kernel panic and halt the system
 * @param file File name where panic occurred (__FILE__)
 * @param line Line number where panic occurred (__LINE__)
 * @param func Function name where panic occurred (__func__)
 * @param fmt Format string for error message
 * @param ... Format arguments
 * @note This function never returns ([[noreturn]])
 */
[[noreturn]] void panic(const char* file,
						int line,
						const char* func,
						const char* fmt,
						...);

/**
 * @brief Macro to trigger a kernel panic
 * @param fmt Format string for error message
 * @param ... Format arguments
 * @note File name, line number, and function name are automatically recorded
 */
#define PANIC(fmt, ...)                                                             \
	kernel::panic(__FILE__, __LINE__, __func__, fmt, ##__VA_ARGS__)

/**
 * @brief Assertion macro that triggers panic if condition is false
 * @param condition Condition to check
 */
#define ASSERT(condition)                                                           \
	do {                                                                            \
		if (!(condition)) {                                                         \
			PANIC("Assertion failed: %s", #condition);                              \
		}                                                                           \
	} while (0)

/**
 * @brief Debug-only assertion
 * @param condition Condition to check
 * @note Does nothing in release builds
 */
#ifdef DEBUG
#define DEBUG_ASSERT(condition) ASSERT(condition)
#else
#define DEBUG_ASSERT(condition) ((void)0)
#endif

/**
 * @brief Assert that a pointer is not NULL
 * @param ptr Pointer to check
 */
#define ASSERT_NOT_NULL(ptr)                                                        \
	do {                                                                            \
		if ((ptr) == nullptr) {                                                     \
			PANIC("Null pointer: %s", #ptr);                                        \
		}                                                                           \
	} while (0)

/**
 * @brief Assert that a value is within the specified range
 * @param value Value to check
 * @param min Minimum value (inclusive)
 * @param max Maximum value (inclusive)
 */
#define ASSERT_IN_RANGE(value, min, max)                                            \
	do {                                                                            \
		if ((value) < (min) || (value) > (max)) {                                   \
			PANIC("Value %s = %ld out of range [%ld, %ld]", #value, (long)(value),  \
				  (long)(min), (long)(max));                                        \
		}                                                                           \
	} while (0)

/**
 * @brief Macro to indicate unimplemented functionality
 */
#define NOT_IMPLEMENTED() PANIC("Not implemented")

/**
 * @brief Macro to indicate unreachable code path
 * @note If this macro is reached, there is a logic error in the program
 */
#define UNREACHABLE() PANIC("Unreachable code reached")

} // namespace kernel