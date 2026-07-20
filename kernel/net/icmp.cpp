#include "net/icmp.hpp"
#include <cstddef>
#include <cstdint>
#include <cstring>
#include "log/log.hpp"
#include "net/checksum.hpp"
#include "net/ethernet.hpp"
#include "net/ipv4.hpp"

namespace kernel::net
{
void handle_icmp_echo_request(const kernel::net::ICMPEchoHeader* echo_header,
							  uint32_t src_ip,
							  size_t icmp_len)
{
	if (icmp_len < sizeof(ICMPEchoHeader)) {
		LOG_ERROR("handle_icmp_echo_request: icmp_len too small: %zu", icmp_len);
		return;
	}

	size_t payload_len = icmp_len - sizeof(ICMPEchoHeader);

	if (sizeof(ICMPEchoHeader) + payload_len > ETHERNET_MTU) {
		LOG_ERROR("handle_icmp_echo_request: reply too large: %zu", icmp_len);
		return;
	}

	uint8_t reply_buf[ETHERNET_MTU];

	ICMPEchoHeader* reply = reinterpret_cast<ICMPEchoHeader*>(reply_buf);
	memcpy(reply, echo_header, icmp_len);

	reply->icmp_header.type = static_cast<uint8_t>(ICMPType::ECHO_REPLY);
	reply->icmp_header.checksum = 0;
	reply->icmp_header.code = 0;

	reply->icmp_header.checksum = calculate_checksum(reply, icmp_len);

	transmit_ipv4_packet(src_ip, IPv4Protocol::ICMP, reply, icmp_len);
}

void process_icmp(const ICMPHeader& icmp_header, uint32_t src_ip, size_t icmp_len)
{
	switch (static_cast<ICMPType>(icmp_header.type)) {
		case ICMPType::ECHO_REQUEST:
			handle_icmp_echo_request(
					reinterpret_cast<const ICMPEchoHeader*>(&icmp_header), src_ip,
					icmp_len);
			break;
		case ICMPType::ECHO_REPLY:
			LOG_ERROR("ICMP Echo Reply received");
			break;
		case ICMPType::DEST_UNREACHABLE:
			LOG_ERROR("ICMP Destination Unreachable received");
			break;
		case ICMPType::TIME_EXCEEDED:
			LOG_ERROR("ICMP Time Exceeded received");
			break;
		default:
			LOG_ERROR("Unknown ICMP type received");
			break;
	}
}
} // namespace kernel::net
