/**
 * @file task/ipc.hpp
 * @brief Inter-process communication (IPC) interface
 * 
 * This file provides IPC functionality for sending and receiving messages between tasks.
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
 * @brief Send a message to the specified task
 * @param dst Destination task ID
 * @param m Message to send (may be modified during sending)
 * @return OK on success, error code on failure
 * @retval OK Message sent successfully
 * @retval ERR_INVALID_ARG Invalid destination ID (-1 or same as sender)
 * @retval ERR_INVALID_TASK Destination task does not exist
 * @note Supports out-of-line memory transfer
 * @note [[gnu::no_caller_saved_registers]] attribute optimizes register saving on call
 */
[[gnu::no_caller_saved_registers]] error_t send_message(ProcessId dst, message& m);

} // namespace kernel::task
