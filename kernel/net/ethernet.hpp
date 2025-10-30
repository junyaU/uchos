/**
 * @file net/ethernet.hpp
 * @brief Ethernet Frame Definitions
 *
 * This file defines the structure of an Ethernet frame and related constants.
 */

#pragma once

#include <cstdint>

namespace kernel::net
{
// Ethernet Frame Types (EtherType)
constexpr uint16_t ETHERTYPE_IPV4 = 0x0800;
constexpr uint16_t ETHERTYPE_ARP = 0x0806;
constexpr uint16_t ETHERTYPE_IPV6 = 0x86DD;

/**
 * @brief Ethernet Frame Header
 */
struct EthernetFrame {
	uint8_t dst_mac[6]; ///< Destination MAC address
	uint8_t src_mac[6]; ///< Source MAC address
	uint16_t ethertype; ///< EtherType (big-endian)
	uint8_t payload[];	///< Frame payload
} __attribute__((packed));

} // namespace kernel::net
