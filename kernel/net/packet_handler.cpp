/**
 * @file net/packet_handler.cpp
 * @brief Network Packet Handler Implementation
 */

#include "net/packet_handler.hpp"
#include <libs/common/endian.hpp>
#include <libs/common/message.hpp>
#include "graphics/log.hpp"
#include "memory/slab.hpp"
#include "net/arp.hpp"
#include "net/ethernet.hpp"
#include "net/ipv4.hpp"
#include "task/task.hpp"

namespace kernel::net
{

void handle_recv_packet(const Message& m)
{
	const EthernetFrame* frame = reinterpret_cast<const EthernetFrame*>(
			m.data.net.packet_data);

	switch (static_cast<EthernetFrameType>(ntohs(frame->ethertype))) {
		case EthernetFrameType::ARP:
			process_arp(*reinterpret_cast<const ARPPacket*>(frame->payload));
			break;
		case EthernetFrameType::IPV4:
			process_ipv4(*reinterpret_cast<const IPv4Header*>(frame->payload));
			break;
		case EthernetFrameType::IPV6:
			LOG_ERROR("IPv6 packet received");
			break;
		default:
			LOG_ERROR("Unknown Ethertype");
			break;
	}
}

void handle_send_packet(const Message& m)
{
	// Handle sending network packet
	LOG_ERROR("Send network packet");
}

void packet_handler_service()
{
	kernel::task::Task* t = kernel::task::CURRENT_TASK;

	t->add_msg_handler(MsgType::IPC_NET_RECV_PACKET, handle_recv_packet);
	t->add_msg_handler(MsgType::IPC_NET_SEND_PACKET, handle_send_packet);

	kernel::task::process_messages(t);
}

} // namespace kernel::net
