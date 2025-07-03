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
[[noreturn]] void task_idle();

/**
 * @brief Main kernel task
 *
 * This task handles kernel-level operations and system management.
 * It may process deferred work, handle system maintenance, or
 * coordinate other kernel activities.
 */
void task_kernel();

/**
 * @brief Shell task for user interaction
 *
 * This task implements the command-line shell interface, allowing
 * users to interact with the system, execute commands, and manage
 * processes.
 */
void task_shell();

/**
 * @brief USB event handler task
 *
 * This task processes USB events and manages USB device communication.
 * It handles device enumeration, data transfers, and USB protocol
 * management for connected devices.
 */
void task_usb_handler();

} // namespace kernel::task
