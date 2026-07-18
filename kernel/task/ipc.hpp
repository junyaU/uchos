/**
 * @file task/ipc.hpp
 * @brief Inter-process communication (IPC) interface
 *
 * This file provides IPC functionality for sending and receiving messages between
 * tasks.
 *
 * @date 2024
 */

#pragma once

#include <libs/common/message.hpp>
#include <libs/common/process_id.hpp>
#include <libs/common/types.hpp>

namespace kernel::task
{

/**
 * @brief Maximum number of messages queued per task
 *
 * Backstop against unbounded queue growth (and thus unbounded heap
 * allocation from interrupt context) when a receiver never drains its
 * queue. A fixed-size ring buffer is planned for Phase 2 (issue #313).
 */
constexpr size_t MAX_QUEUED_MESSAGES = 1024;

/**
 * @brief Send a message to the specified task
 * @param dst Destination task ID
 * @param m Message to send (may be modified during sending)
 * @return OK on success, error code on failure
 * @retval OK Message sent successfully
 * @retval ERR_INVALID_ARG Invalid destination ID (-1 or same as sender)
 * @retval ERR_INVALID_TASK Destination task does not exist
 * @retval ERR_QUEUE_FULL Destination queue reached MAX_QUEUED_MESSAGES
 * @note Supports out-of-line memory transfer
 * @note [[gnu::no_caller_saved_registers]] lets interrupt handlers call this
 * without spilling every register
 * @warning Because of that attribute the return value is NOT reliable at the
 * call site (RAX is restored by the epilogue); treat errors as diagnostics
 * (they are logged) rather than branching on them
 */
[[gnu::no_caller_saved_registers]] error_t send_message(ProcessId dst, Message& m);

} // namespace kernel::task
