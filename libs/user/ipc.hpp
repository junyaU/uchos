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

/**
 * @brief Answer a received request (server side of an RPC, IPC_REPLY)
 *
 * Echoes the request's correlation so the kernel can pair the reply with
 * the caller blocked in call(); the kernel stamps MSG_FLAG_REPLY and the
 * true sender at the syscall boundary (issue #315 3b-10). A request with
 * correlation 0 expects no reply and makes this a no-op. Put the
 * server-side outcome in resp->result.
 *
 * @param req The request being answered
 * @param resp Reply message (type/payload/result set by the caller)
 * @return OK on delivery (or no-op), negative error_t otherwise
 */
error_t reply_message(const Message* req, Message* resp);

void initialize_task();

/**
 * @brief Release an OOL region received with a message
 *
 * Every received Message whose ool.size is nonzero maps a buffer into this
 * process at ool.addr; pass that address here after use, or the pages stay
 * mapped until the process exits.
 */
void ool_release(const void* addr);
