#include "net/ipv4.hpp"
#include <cstddef>
#include <cstring>
#include <libs/common/endian.hpp>
#include "graphics/log.hpp"
#include "hardware/virtio/net.hpp"
#include "net/arp.hpp"
#include "net/checksum.hpp"
#include "net/ethernet.hpp"
#include "net/icmp.hpp"

namespace kernel::net
{
void process_ipv4(const IPv4Header& ip_header)
{
	size_t header_len = (ip_header.version_ihl & 0x0F) * 4;
	size_t total_len = ntohs(ip_header.total_length);
	size_t payload_len = total_len - header_len;

	const uint8_t* payload =
			reinterpret_cast<const uint8_t*>(&ip_header) + header_len;

	switch (static_cast<IPv4Protocol>(ip_header.protocol)) {
		case IPv4Protocol::ICMP:
			process_icmp(*reinterpret_cast<const ICMPHeader*>(payload),
						 ntohl(ip_header.src_ip), payload_len);
			break;
		case IPv4Protocol::TCP:
			LOG_ERROR("TCP packet received");
			break;
		case IPv4Protocol::UDP:
			LOG_ERROR("UDP packet received");
			break;
		default:
			LOG_ERROR("Unknown IPv4 protocol");
			break;
	}
}

void transmit_ipv4_packet(uint32_t dst_ip,
						  IPv4Protocol protocol,
						  const void* payload,
						  size_t payload_len)
{
	IPv4Header ip_header;
	size_t total_len = sizeof(IPv4Header) + payload_len;
	ip_header.version_ihl = 0x45;
	ip_header.dscp_ecn = 0;
	ip_header.total_length = htons(static_cast<uint16_t>(total_len));
	ip_header.ttl = DEFAULT_TTL;
	ip_header.protocol = static_cast<uint8_t>(protocol);
	ip_header.src_ip = htonl(hw::virtio::MY_IP);
	ip_header.dst_ip = htonl(dst_ip);
	ip_header.header_checksum = 0;

	ip_header.header_checksum = calculate_checksum(&ip_header, sizeof(IPv4Header));

	uint8_t packet_buffer[sizeof(IPv4Header) + payload_len];
	memcpy(packet_buffer, &ip_header, sizeof(IPv4Header));
	memcpy(packet_buffer + sizeof(IPv4Header), payload, payload_len);

	uint8_t dst_mac[6] = { 0 };
	if (!arp_table.resolve(dst_ip, dst_mac)) {
		LOG_ERROR("Failed to resolve MAC address for IP: %08x", dst_ip);
		return;
	}

	transmit_ethernet_frame(dst_mac, EthernetFrameType::IPV4, packet_buffer,
							total_len);
}
} // namespace kernel::net
