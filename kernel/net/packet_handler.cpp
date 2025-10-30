/**
 * @file net/packet_handler.cpp
 * @brief Network Packet Handler Implementation
 */

#include "packet_handler.hpp"
#include <libs/common/message.hpp>
#include "graphics/log.hpp"
#include "task/task.hpp"

namespace kernel::net
{

void handle_recv_packet(const Message& m)
{
	// Handle received network packet
	LOG_ERROR("Received network packet");
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
