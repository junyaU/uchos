#include "net/ipv4.hpp"
#include <libs/common/endian.hpp>
#include "graphics/log.hpp"
#include "net/icmp.hpp"

namespace kernel::net
{
void process_ipv4(const IPv4Header& ip_header)
{
	switch (static_cast<IPv4Protocol>(ip_header.protocol)) {
		case IPv4Protocol::ICMP:
			process_icmp(*reinterpret_cast<const ICMPHeader*>(
					reinterpret_cast<const uint8_t*>(&ip_header) + sizeof(IPv4Header)));
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
} // namespace kernel::net
