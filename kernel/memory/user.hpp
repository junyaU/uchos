/**
 * @file memory/user.hpp
 * @brief User space memory access utilities
 *
 * This file provides safe functions for copying data between kernel and
 * user space. These functions include proper validation and error handling
 * to prevent security vulnerabilities and kernel crashes.
 *
 * @date 2024
 */

#pragma once

#include <cstddef>

/**
 * @brief Annotation for user space pointers
 *
 * This macro is used to annotate pointers that point to user space memory.
 * It serves as documentation and can be used by static analysis tools.
 */
#define __user

/**
 * @brief Check if an address range is in user space
 *
 * Validates that the given address range is entirely within user space
 * memory. This is a critical security check to prevent kernel memory
 * from being accessed through user space interfaces.
 *
 * @param addr Starting address to check
 * @param n Number of bytes to check from addr
 * @return true if the entire range is in user space, false otherwise
 *
 * @note The function checks both the start and end of the range
 */
bool is_user_address(const void* addr, size_t n);

/**
 * @brief Copy data from kernel space to user space
 *
 * Safely copies data from a kernel buffer to a user space buffer.
 * The function validates the user space address before copying.
 *
 * @param to User space destination buffer
 * @param from Kernel space source buffer
 * @param n Number of bytes to copy
 * @return Number of bytes NOT copied (0 on success)
 *
 * @note Returns non-zero if the copy fails (e.g., invalid user address)
 * @warning The user space buffer must be writable
 */
size_t copy_to_user(void __user* to, const void* from, size_t n);

/**
 * @brief Copy data from user space to kernel space
 *
 * Safely copies data from a user space buffer to a kernel buffer.
 * The function validates the user space address before copying.
 *
 * @param to Kernel space destination buffer
 * @param from User space source buffer
 * @param n Number of bytes to copy
 * @return Number of bytes NOT copied (0 on success)
 *
 * @note Returns non-zero if the copy fails (e.g., invalid user address)
 * @warning The kernel buffer must be large enough to hold n bytes
 */
size_t copy_from_user(void* to, const void __user* from, size_t n);
