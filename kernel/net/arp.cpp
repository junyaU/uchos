#include "net/arp.hpp"
#include <cstring>
#include "graphics/log.hpp"
#include "hardware/virtio/net.hpp"
#include "libs/common/endian.hpp"
#include "memory/slab.hpp"
#include "net/ethernet.hpp"

namespace kernel::net
{
ARPTable arp_table;

ARPTable::ARPTable() { clear(); }

int ARPTable::find_empty_slot()
{
	for (size_t i = 0; i < MAX_ENTRIES; ++i) {
		if (!entries_[i].valid) {
			return i;
		}
	}
	return -1;
}

bool ARPTable::resolve(uint32_t ip_addr, uint8_t* mac_addr)
{
	for (size_t i = 0; i < MAX_ENTRIES; ++i) {
		if (entries_[i].valid && entries_[i].ip_addr == ip_addr) {
			memcpy(mac_addr, entries_[i].mac_addr, MAC_ADDR_SIZE);
			return true;
		}
	}
	return false;
}

void ARPTable::add(uint32_t ip_addr, const uint8_t* mac_addr)
{
	for (size_t i = 0; i < MAX_ENTRIES; ++i) {
		if (entries_[i].valid && entries_[i].ip_addr == ip_addr) {
			memcpy(entries_[i].mac_addr, mac_addr, MAC_ADDR_SIZE);
			return;
		}
	}

	int slot = find_empty_slot();
	if (slot != -1) {
		entries_[slot].ip_addr = ip_addr;
		memcpy(entries_[slot].mac_addr, mac_addr, MAC_ADDR_SIZE);
		entries_[slot].valid = true;
	}
}

void ARPTable::remove(uint32_t ip_addr)
{
	for (size_t i = 0; i < MAX_ENTRIES; ++i) {
		if (entries_[i].valid && entries_[i].ip_addr == ip_addr) {
			entries_[i].valid = false;
			return;
		}
	}
}

void ARPTable::clear()
{
	for (size_t i = 0; i < MAX_ENTRIES; ++i) {
		entries_[i].valid = false;
		entries_[i].ip_addr = 0;
		memset(entries_[i].mac_addr, 0, MAC_ADDR_SIZE);
	}
}

void handle_arp_request(const ARPPacket& arp_packet)
{
	if (arp_packet.target_ip != htonl(hw::virtio::MY_IP)) {
		return;
	}

	uint32_t sender_ip = ntohl(arp_packet.sender_ip);
	arp_table.add(sender_ip, arp_packet.sender_mac);

	void* buf;
	ALLOC_OR_RETURN(buf, sizeof(ARPPacket), kernel::memory::ALLOC_ZEROED);

	ARPPacket& reply = *reinterpret_cast<ARPPacket*>(buf);
	reply.hw_type = htons(1);			 // Ethernet
	reply.protocol_type = htons(0x0800); // IPv4
	reply.hw_size = MAC_ADDR_SIZE;
	reply.protocol_size = 4;
	reply.opcode = htons(static_cast<uint16_t>(ARPOpcode::REPLY));
	reply.sender_ip = arp_packet.target_ip;
	reply.target_ip = arp_packet.sender_ip;
	memcpy(reply.sender_mac, hw::virtio::mac_addr, MAC_ADDR_SIZE);
	memcpy(reply.target_mac, arp_packet.sender_mac, MAC_ADDR_SIZE);

	transmit_ethernet_frame(arp_packet.sender_mac, EthernetFrameType::ARP, &reply,
							sizeof(ARPPacket));
}

void handle_arp_reply(const ARPPacket& arp_packet)
{
	// Handle ARP reply
	LOG_ERROR("ARP Reply received");
}

void process_arp(const ARPPacket& arp_packet)
{
	switch (static_cast<ARPOpcode>(ntohs(arp_packet.opcode))) {
		case ARPOpcode::REQUEST:
			handle_arp_request(arp_packet);
			break;
		case ARPOpcode::REPLY:
			handle_arp_reply(arp_packet);
			break;
		default:
			LOG_ERROR("Unknown ARP opcode");
			break;
	}
}
} // namespace kernel::net
