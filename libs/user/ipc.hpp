#pragma once

#include <libs/common/message.hpp>
#include <libs/common/process_id.hpp>
#include <libs/common/types.hpp>

void receive_message(Message* msg);

void send_message(ProcessId dst, const Message* msg);

/**
 * @brief Build a request message stamped with this process as the sender
 * @param type Message type of the request
 * @return Message with type and sender filled in
 */
Message make_request(MsgType type);

/**
 * @brief Send a request and block until its reply arrives (IPC_CALL)
 *
 * The kernel assigns a correlation id and delivers the matching reply into
 * a dedicated slot, so the answer can never be confused with other
 * messages (issue #314 Stage B). Check the reply's result field for the
 * server-side outcome.
 *
 * @param dst Destination process
 * @param msg Request to send (sender must already be set)
 * @return The reply message
 */
Message call(ProcessId dst, Message* msg);

void initialize_task();

void deallocate_ool_memory(ProcessId sender, void* addr, size_t size);
