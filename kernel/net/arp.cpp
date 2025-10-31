#include "net/arp.hpp"
#include <cstring>
#include "graphics/log.hpp"
#include "hardware/virtio/net.hpp"
#include "libs/common/endian.hpp"
#include "net/ethernet.hpp"

namespace kernel::net
{
ARPTable arp_table;

void handle_arp_request(ARPPacket* arp_packet)
{
	if (arp_packet->target_ip != htonl(hw::virtio::MY_IP)) {
		return;
	}

	ARPPacket reply;
	reply.hw_type = htons(1);			 // Ethernet
	reply.protocol_type = htons(0x0800); // IPv4
	reply.hw_size = MAC_ADDR_SIZE;
	reply.protocol_size = 4;
	reply.opcode = htons(static_cast<uint16_t>(ARPOpcode::REPLY));
	reply.sender_ip = arp_packet->target_ip;
	reply.target_ip = arp_packet->sender_ip;
	memcpy(reply.sender_mac, hw::virtio::mac_addr, MAC_ADDR_SIZE);
	memcpy(reply.target_mac, arp_packet->sender_mac, MAC_ADDR_SIZE);

	transmit_ethernet_frame(arp_packet->sender_mac, EthernetFrameType::ARP, &reply,
							sizeof(ARPPacket));
}

void handle_arp_reply(ARPPacket* arp_packet)
{
	// Handle ARP reply
	LOG_ERROR("ARP Reply received");
}

void process_arp(ARPPacket* arp_packet)
{
	switch (static_cast<ARPOpcode>(ntohs(arp_packet->opcode))) {
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
