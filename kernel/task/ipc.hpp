/**
 * @file task/ipc.hpp
 * @brief Inter-process communication (IPC) interface
 *
 * Two delivery mechanisms with different contracts (issue #314 Stage A):
 *
 * - Messages: bounded FIFO ring per task (task/message_queue.hpp), pushed
 *   by send_message() from task context only.
 * - Notifications: one sticky doorbell bit per NotifyType, set by notify().
 *   The only mechanism interrupt handlers may use: O(1), no allocation,
 *   never lost (repeated notifies coalesce into one pending bit).
 *
 * Receivers drain both through try_receive()/receive_blocking(): pending
 * notification bits are synthesized into NOTIFY_* messages first, then the
 * ring is popped in FIFO order.
 *
 * @date 2024
 */

#pragma once

#include <libs/common/message.hpp>
#include <libs/common/process_id.hpp>
#include <libs/common/types.hpp>

namespace kernel::task
{

struct Task;

/**
 * @brief Doorbell classes deliverable from interrupt context
 *
 * Each value is one sticky bit in Task::pending_notifications. The order
 * defines the bit positions and the drain priority (lowest bit first).
 */
enum class NotifyType : uint8_t {
	XHCI,
	VIRTIO_BLK,
	VIRTIO_NET_RX,
	VIRTIO_NET_TX,
	TIMER,
	COUNT, // must be the last
};

/**
 * @brief Bit mask for one notification type in Task::pending_notifications
 */
constexpr uint32_t notify_bit(NotifyType type)
{
	return 1U << static_cast<uint8_t>(type);
}

/**
 * @brief Send a message to the specified task
 * @param dst Destination task ID
 * @param m Message to send (may be modified during sending)
 * @return OK on success, error code on failure
 * @retval OK Message sent successfully
 * @retval ERR_INVALID_ARG Invalid destination ID (-1 or same as sender)
 * @retval ERR_INVALID_TASK Destination task does not exist
 * @retval ERR_QUEUE_FULL Destination ring reached MessageQueue::CAPACITY
 * @note Task context only; interrupt handlers must use notify() instead.
 * The return value is reliable (the ISR-only register-preservation
 * attribute moved to notify() with issue #314), so callers can branch on it.
 * @note Supports out-of-line memory transfer
 */
error_t send_message(ProcessId dst, Message& m);

/**
 * @brief Raise a doorbell notification for the destination task
 *
 * Sets the sticky bit for the given type and wakes the destination if it is
 * sleeping in a receive, or in wait_notification() with a matching mask.
 * Safe from interrupt context: no allocation, no ring access, idempotent.
 *
 * @param dst Destination task ID (a vanished task is silently ignored)
 * @param type Doorbell to raise
 * @note [[gnu::no_caller_saved_registers]] lets interrupt handlers call
 * this without spilling every register
 */
[[gnu::no_caller_saved_registers]] void notify(ProcessId dst, NotifyType type);

/**
 * @brief Non-blocking receive: pending notifications first, then the ring
 *
 * Notification bits are converted into synthesized NOTIFY_* messages with
 * sender = INTERRUPT, one bit per call, lowest bit first.
 *
 * @param t Task whose queue to drain
 * @param out Received message on success
 * @return true when a message was delivered, false when nothing is pending
 */
bool try_receive(Task* t, Message* out);

/**
 * @brief Blocking receive for the current task
 *
 * Sleeps in TASK_WAITING (WaitReason::RECEIVE) while nothing is pending;
 * senders and notifiers wake it. Never spins (issue #314 Stage A).
 *
 * @return The received (or synthesized) message
 */
Message receive_blocking();

/**
 * @brief Block the current task until one of the masked doorbells is raised
 *
 * Consumes the masked bits on return. Returns immediately when one is
 * already pending, which closes the "interrupt fired before the task went
 * to sleep" race that the previous bare switch_next_task(true) wait had.
 * While waiting, incoming messages are queued without waking the task, so
 * a mid-wait request can no longer cause a spurious resume.
 *
 * @param mask OR of notify_bit() values to wait for
 */
void wait_notification(uint32_t mask);

} // namespace kernel::task
