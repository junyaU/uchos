/**
 * @file builtin.hpp
 * @brief Built-in kernel tasks for core system functionality
 */

#pragma once

namespace kernel::task
{

/**
 * @brief The system idle task
 *
 * This task runs when no other tasks are ready to execute. It puts
 * the CPU into a low-power state (typically using HLT instruction)
 * until an interrupt occurs.
 *
 * @note This function never returns
 * @note Has the lowest priority in the system
 */
[[noreturn]] void idle_service();

/**
 * @brief Main kernel task
 *
 * This task handles kernel-level operations and system management.
 * It may process deferred work, handle system maintenance, or
 * coordinate other kernel activities.
 */
void kernel_service();

} // namespace kernel::task
