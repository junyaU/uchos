/**
 * @file interrupt/handler.h
 * @brief Low-level interrupt handler functions
 *
 * This file declares interrupt handler functions that are called from
 * assembly code when interrupts occur. These functions provide the
 * interface between the low-level interrupt entry points and the
 * higher-level C++ interrupt handling code.
 *
 * @date 2024
 */

#pragma once

extern "C" {
/**
 * @brief Timer interrupt handler
 *
 * This function is called when a timer interrupt occurs. It handles
 * timer tick processing, updates system time, and may trigger task
 * scheduling if needed.
 *
 * @note Called from assembly interrupt handler code
 * @note Runs in interrupt context with interrupts disabled
 */
void on_timer_interrupt();

/**
 * @brief Perform task switch from interrupt context
 *
 * This function implements the task switching logic when called from
 * an interrupt handler. It saves the interrupted task's context and
 * switches to the next scheduled task.
 *
 * @note Must only be called from interrupt context
 * @note Interrupts must be disabled when calling this function
 */
void interrupt_task_switch();
}