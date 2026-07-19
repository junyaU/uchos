/**
 * @file interrupt/routing.hpp
 * @brief IRQ-to-task notification routing table
 *
 * Drivers register "vector X belongs to me" at initialization time, and the
 * interrupt handlers deliver doorbells through the table. This removes all
 * knowledge of specific services (destination PIDs) from the interrupt
 * layer (issue #314 Stage A).
 */

#pragma once

#include <cstdint>
#include <libs/common/process_id.hpp>
#include <libs/common/types.hpp>
#include "interrupt/vector.hpp"
#include "task/ipc.hpp"

namespace kernel::interrupt
{

/**
 * @brief Wire an interrupt vector to a task's doorbell
 *
 * Called by the driver that owns the vector, from its own service task
 * (typically with CURRENT_TASK->id as @p dst). Re-registering a vector
 * overwrites the previous route.
 *
 * @param vector Interrupt vector to route
 * @param dst Task to notify when the vector fires
 * @param type Doorbell bit to raise on delivery
 * @return OK on success, ERR_INVALID_ARG for an out-of-range vector
 */
error_t register_irq_notification(InterruptVector::Number vector,
								  ProcessId dst,
								  kernel::task::NotifyType type);

/**
 * @brief Deliver the doorbell registered for a vector
 *
 * Called from interrupt handlers. An unregistered vector is counted and
 * otherwise ignored; no allocation, no logging.
 *
 * @note [[gnu::no_caller_saved_registers]] lets interrupt handlers call
 * this without spilling every register
 */
[[gnu::no_caller_saved_registers]] void notify_irq(InterruptVector::Number vector);

/**
 * @brief Number of interrupts that fired with no registered route
 *
 * Diagnostic counter for tests and debugging; a steadily growing value
 * means a driver forgot to register its vector.
 */
uint64_t unrouted_irq_count();

} // namespace kernel::interrupt
