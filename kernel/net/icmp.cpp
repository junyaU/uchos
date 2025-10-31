#include "net/icmp.hpp"
#include "graphics/log.hpp"

namespace
{
void handle_icmp_echo_request(kernel::net::ICMPEchoHeader* echo_header,
							  size_t ip_total_len)
{
	LOG_ERROR("ICMP Echo Request received");
	// Here you would typically construct and send an ICMP Echo Reply
}

} // namespace

namespace kernel::net
{
void process_icmp(ICMPHeader* icmp_header)
{
	switch (static_cast<ICMPType>(icmp_header->type)) {
		case ICMPType::ECHO_REQUEST:
			LOG_ERROR("ICMP Echo Request received");
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
