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
#include "memory/slab.hpp"

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
 * @brief Synchronous RPC: send a request and block until its reply
 *
 * The kernel assigns a per-task correlation id to the request and parks the
 * caller in WAITING (WaitReason::REPLY). The reply bypasses the message
 * ring and lands in the caller's dedicated reply slot, so it can never be
 * confused with other traffic and never disturbs queued messages.
 *
 * The call graph must stay acyclic (user → {KERNEL, FS}, FS → BLK);
 * a service must never call() one of its clients.
 *
 * @param dst Destination (server) task
 * @param inout Request on entry; overwritten with the reply on success
 * @return OK when a reply was delivered (check inout->result for the
 * server-side outcome), or the send_message() delivery error
 */
error_t call(ProcessId dst, Message* inout);

/**
 * @brief Reply to a received request (server side)
 *
 * Inherits the request's correlation and stamps MSG_FLAG_REPLY, then
 * delivers to the requester's reply slot. A request with correlation 0
 * expects no reply and makes this a no-op. Every request with a nonzero
 * correlation must be answered exactly once — success or error — with the
 * outcome in resp->result.
 *
 * @param req The request being answered
 * @param resp Reply message (type/payload set by the caller)
 * @return OK on delivery (or no-op), error code otherwise; a reply nobody
 * is waiting for is dropped with a log (protocol bug made visible)
 */
error_t reply(const Message& req, Message* resp);

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

/**
 * @brief Allocate an OOL payload buffer destined for a user task
 *
 * Page-aligned and backed by dedicated pages (never a shared slab page), so
 * mapping it into a user address space can leak nothing but the payload.
 * Kernel-to-kernel OOL payloads that are never mapped may use plain
 * alloc()/make_kbuf() instead.
 *
 * @param size Payload bytes (> 0)
 * @return Owning buffer, empty on allocation failure
 */
kernel::memory::unique_kbuf<> make_ool_buffer(size_t size);

/**
 * @brief Free the kernel-owned OOL buffer attached to m, if any
 *
 * Cleanup path for undelivered or drained messages (send failure, exit-time
 * ring drain); a normal receiver takes ownership instead.
 */
void free_message_ool(Message& m);

/**
 * @brief Copy a user-space OOL payload into a kernel buffer (send boundary)
 *
 * Replaces m->ool.addr (user vaddr) with a fresh kernel buffer holding a
 * copy; the message then owns that buffer like any kernel-side OOL payload.
 *
 * @return OK (also when there is no payload), ERR_OOL_LIMIT above
 * OOL_MAX_SIZE, ERR_INVALID_ARG for a bad address, ERR_NO_MEMORY
 */
error_t copy_in_ool_from_user(Message* m);

/**
 * @brief Map m's kernel OOL buffer into t's space (receive boundary)
 *
 * Rewrites m->ool.addr to the user vaddr and records the region in
 * t->ool_regions for ool_release()/exit cleanup. When the region table is
 * full or mapping fails, the payload is freed and the message is delivered
 * anyway with result rewritten (ERR_OOL_LIMIT / ERR_NO_MEMORY).
 */
void deliver_ool_to_user(Task* t, Message* m);

/**
 * @brief IPC_OOL_RELEASE: unmap and free the region mapped at uaddr
 * @return OK, or ERR_INVALID_ARG when uaddr names no live region
 */
error_t release_ool_region(Task* t, uint64_t uaddr);

/**
 * @brief Free every OOL buffer a task still owns (exit path)
 *
 * Covers the mapped-region table, undelivered ring messages and a pending
 * reply slot. The user-space mappings themselves die with the page table,
 * so this only releases the physical buffers.
 */
void release_all_ool(Task* t);

/**
 * @brief Free the mapped-region buffers only (exec path)
 *
 * exec destroys the old user image (and with it every OOL mapping) but the
 * task lives on, so queued messages must survive.
 */
void release_ool_regions(Task* t);

} // namespace kernel::task
