/**
 * @file net/ipv4.hpp
 * @brief IPv4 Packet Definitions
 *
 * This file defines the structure of an IPv4 packet and related constants.
 */

#pragma once

#include <cstdint>
namespace kernel::net
{

/**
 * @brief IPv4 Protocol Numbers
 */
enum class IPv4Protocol : uint8_t {
	ICMP = 0x01,
	TCP = 0x06,
	UDP = 0x11,
};

constexpr uint32_t IPV4_BROADCAST_ADDR = 0xFFFFFFFF;
constexpr uint32_t IPV4_ADDR_UNSPECIFIED = 0x00000000;

/**
 * @brief IPv4 Header Structure
 */
struct IPv4Header {
	uint8_t version_ihl;			///< Version and Internet Header Length
	uint8_t dscp_ecn;				///< DSCP and ECN
	uint16_t total_length;			///< Total Length (header + data)
	uint16_t id;					///< Identification
	uint16_t flags_fragment_offset; ///< Flags and Fragment Offset
	uint8_t ttl;					///< Time to Live (routing hop limit)
	uint8_t protocol;				///< Protocol
	uint16_t header_checksum;		///< Header Checksum
	uint32_t src_ip;				///< Source IP Address
	uint32_t dst_ip;				///< Destination IP Address
} __attribute__((packed));

void process_ipv4(const IPv4Header& ip_header);

} // namespace kernel::net
