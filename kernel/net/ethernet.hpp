/**
 * @file net/ethernet.hpp
 * @brief Ethernet Frame Definitions
 *
 * This file defines the structure of an Ethernet frame and related constants.
 */

#pragma once

#include <cstddef>
#include <cstdint>
#include "libs/common/types.hpp"

namespace kernel::net
{
enum class EthernetFrameType : uint16_t {
	IPV4 = 0x0800,
	ARP = 0x0806,
	IPV6 = 0x86DD,
};

/**
 * @brief Ethernet Frame Header
 */
struct EthernetFrame {
	uint8_t dst_mac[6]; ///< Destination MAC address
	uint8_t src_mac[6]; ///< Source MAC address
	uint16_t ethertype; ///< EtherType (big-endian)
	uint8_t payload[];	///< Frame payload
} __attribute__((packed));

// 同じLAN内の通信のみ想定
error_t transmit_ethernet_frame(const uint8_t* dst_mac,
								EthernetFrameType type,
								const void* payload,
								size_t payload_len);

} // namespace kernel::net
