/**
 * @file endian.hpp
 * @brief Byte order conversion utilities
 * @date 2025-01-30
 *
 * Provides functions for converting between host and network byte order.
 * Network byte order is big-endian, while x86_64 is little-endian.
 */
#pragma once

#include <cstdint>

/**
 * @brief Convert 16-bit value from network byte order to host byte order
 *
 * @param value Value in network byte order (big-endian)
 * @return Value in host byte order (little-endian on x86_64)
 */
inline constexpr uint16_t ntohs(uint16_t value)
{
	return ((value & 0xFF) << 8) | ((value >> 8) & 0xFF);
}

/**
 * @brief Convert 16-bit value from host byte order to network byte order
 *
 * @param value Value in host byte order (little-endian on x86_64)
 * @return Value in network byte order (big-endian)
 */
inline constexpr uint16_t htons(uint16_t value) { return ntohs(value); }

/**
 * @brief Convert 32-bit value from network byte order to host byte order
 *
 * @param value Value in network byte order (big-endian)
 * @return Value in host byte order (little-endian on x86_64)
 */
inline constexpr uint32_t ntohl(uint32_t value)
{
	return ((value & 0xFF) << 24) | ((value & 0xFF00) << 8) |
		   ((value >> 8) & 0xFF00) | ((value >> 24) & 0xFF);
}

/**
 * @brief Convert 32-bit value from host byte order to network byte order
 *
 * @param value Value in host byte order (little-endian on x86_64)
 * @return Value in network byte order (big-endian)
 */
inline constexpr uint32_t htonl(uint32_t value) { return ntohl(value); }

/**
 * @brief Convert 64-bit value from network byte order to host byte order
 *
 * @param value Value in network byte order (big-endian)
 * @return Value in host byte order (little-endian on x86_64)
 */
inline constexpr uint64_t ntohll(uint64_t value)
{
	return ((value & 0xFF) << 56) | ((value & 0xFF00) << 40) |
		   ((value & 0xFF0000) << 24) | ((value & 0xFF000000) << 8) |
		   ((value >> 8) & 0xFF000000) | ((value >> 24) & 0xFF0000) |
		   ((value >> 40) & 0xFF00) | ((value >> 56) & 0xFF);
}

/**
 * @brief Convert 64-bit value from host byte order to network byte order
 *
 * @param value Value in host byte order (little-endian on x86_64)
 * @return Value in network byte order (big-endian)
 */
inline constexpr uint64_t htonll(uint64_t value) { return ntohll(value); }
