/**
 * @file net/icmp.hpp
 * @brief ICMP Packet Definitions
 *
 * This file defines the structure of an ICMP packet and related constants.
 */

#pragma once

#include <cstddef>
#include <cstdint>
namespace kernel::net
{

/**
 * @brief ICMP Message Types
 */
enum class ICMPType : uint8_t {
	ECHO_REPLY = 0x00,
	DEST_UNREACHABLE = 0x03,
	REDIRECT = 0x05,
	ECHO_REQUEST = 0x08,
	TIME_EXCEEDED = 0x0B,
	PARAMETER_PROBLEM = 0x0C,
};

constexpr uint32_t ICMP_HEADER_SIZE = 8;

/**
 * @brief ICMP Header
 */
struct ICMPHeader {
	uint8_t type;	   ///< Type
	uint8_t code;	   ///< Code
	uint16_t checksum; ///< Checksum
} __attribute__((packed));

/**
 * @brief ICMP Echo Request/Reply (Type 0 and 8)
 */
struct ICMPEchoHeader {
	ICMPHeader icmp_header;
	uint16_t id;	   ///< Identifier
	uint16_t sequence; ///< Sequence Number
	uint8_t data[];	   ///< Payload Data
} __attribute__((packed));

/**
 * @brief ICMP Destination Unreachable (Type 3)
 */
struct ICMPDestUnreach {
	ICMPHeader header;		 ///< Common ICMP header
	uint16_t unused;		 ///< Unused (must be 0)
	uint16_t next_hop_mtu;	 ///< Next-hop MTU (for code 4)
	uint8_t original_data[]; ///< Original IP header + 8 bytes
} __attribute__((packed));

/**
 * @brief ICMP Time Exceeded (Type 11)
 */
struct ICMPTimeExceeded {
	ICMPHeader header;		 ///< Common ICMP header
	uint32_t unused;		 ///< Unused (must be 0)
	uint8_t original_data[]; ///< Original IP header + 8 bytes
} __attribute__((packed));

void process_icmp(ICMPHeader* icmp_header);

} // namespace kernel::net
