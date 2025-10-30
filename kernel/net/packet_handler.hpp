/**
 * @file net/packet_handler.hpp
 * @brief Network Packet Handler
 *
 * This file defines structures and functions for handling network packets
 * received from VirtIO network device. It implements basic protocol stack
 * including Ethernet, ARP, IP, and ICMP protocols.
 */
#pragma once

#include <cstddef>
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

void packet_handler_service();

} // namespace kernel::net
