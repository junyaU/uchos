#pragma once

#include <libs/common/message.hpp>
#include <libs/common/process_id.hpp>
#include <libs/common/types.hpp>

void receive_message(Message* msg);

void send_message(ProcessId dst, const Message* msg);

Message wait_for_message(MsgType type);

/**
 * @brief Build a request message stamped with this process as the sender
 * @param type Message type of the request
 * @return Message with type and sender filled in
 */
Message make_request(MsgType type);

/**
 * @brief Send a request and block until the reply of the same type arrives
 *
 * Wraps the send_message / wait_for_message pair used by every synchronous
 * service call. Replies are matched by message type (the current IPC has no
 * correlation id — see issue #314); when IPC v2 lands, only this function
 * has to change.
 *
 * @param dst Destination process
 * @param msg Request to send (sender must already be set)
 * @return The reply message
 */
Message call(ProcessId dst, Message* msg);

void initialize_task();

void deallocate_ool_memory(ProcessId sender, void* addr, size_t size);
