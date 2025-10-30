#include "net/ipv4.hpp"
#include "graphics/log.hpp"

namespace kernel::net
{
void process_ipv4(IPv4Header* ip_header)
{
	switch (static_cast<IPv4Protocol>(ip_header->protocol)) {
		case IPv4Protocol::ICMP:
			LOG_ERROR("ICMP packet received");
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
