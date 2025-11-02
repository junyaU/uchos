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
	uint32_t ip_addr;	 ///< IP Address
	uint8_t mac_addr[6]; ///< MAC Address
	bool valid;			 ///< Validity of the entry
};

/**
 * @brief ARP Cache Table
 */
class ARPTable
{
private:
	static constexpr size_t MAX_ENTRIES = 32;
	ARPEntry entries_[MAX_ENTRIES];

	/**
	 * @brief Find an empty slot
	 * @return Index of empty slot, or -1 if full
	 */
	int find_empty_slot();

public:
	ARPTable();

	/**
	 * @brief Resolve IP address to MAC address
	 * @param ip_addr IPv4 address (host byte order)
	 * @param mac_addr Output MAC address buffer
	 * @return true if found, false otherwise
	 */
	bool resolve(uint32_t ip_addr, uint8_t* mac_addr);

	/**
	 * @brief Add or update an ARP entry
	 * @param ip_addr IPv4 address (host byte order)
	 * @param mac_addr MAC address
	 */
	void add(uint32_t ip_addr, const uint8_t* mac_addr);

	/**
	 * @brief Remove an ARP entry
	 * @param ip_addr IPv4 address (host byte order)
	 */
	void remove(uint32_t ip_addr);

	/**
	 * @brief Clear all entries
	 */
	void clear();
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

extern ARPTable arp_table;

void process_arp(const ARPPacket& arp_packet);

} // namespace kernel::net
