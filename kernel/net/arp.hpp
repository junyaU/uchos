/**
 * @file net/arp.hpp
 * @brief ARP Packet Definitions
 *
 * This file defines the structure of an ARP packet and related constants.
 */

#pragma once

#include <cstddef>
#include <cstdint>
namespace kernel::net
{

struct ARPEntry {
	bool is_valid;		 ///< Is the entry valid
	uint32_t ip_addr;	 ///< IP Address
	uint8_t mac_addr[6]; ///< MAC Address
};

struct ARPTable {
	static constexpr size_t MAX_ENTRIES = 128;
	ARPEntry entries[MAX_ENTRIES];
};

enum class ARPOpcode : uint16_t {
	REQUEST = 1,
	REPLY = 2,
};

struct ARPPacket {
	uint16_t hw_type;		///< Hardware Type
	uint16_t protocol_type; ///< Protocol Type
	uint8_t hw_size;		///< Hardware Address Length
	uint8_t protocol_size;	///< Protocol Address Length
	uint16_t opcode;		///< Operation Code
	uint8_t sender_mac[6];	///< Sender MAC Address
	uint32_t sender_ip;		///< Sender IP Address
	uint8_t target_mac[6];	///< Target MAC Address
	uint32_t target_ip;		///< Target IP Address
} __attribute__((packed));

constexpr size_t MAC_ADDR_SIZE = 6;

void process_arp(ARPPacket* arp_packet);

} // namespace kernel::net
