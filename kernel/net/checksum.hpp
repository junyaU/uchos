/**
 * @file net/checksum.hpp
 * @brief Checksum Calculation Functions
 *
 * This file provides functions for calculating checksums for network packets.
 */
#pragma once

#include <cstddef>
#include <cstdint>

namespace kernel::net
{
uint16_t calculate_checksum(const void* data, size_t len);
}
