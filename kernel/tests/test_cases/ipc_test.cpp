/**
 * @file tests/test_cases/ipc_test.cpp
 * @brief Tests for the IPC v2 Stage A primitives (issue #314):
 * fixed-capacity message ring, doorbell notifications, wakeup gating and
 * the IRQ routing table.
 */

#include "tests/test_cases/ipc_test.hpp"
#include <cstdint>
#include <libs/common/message.hpp>
#include <libs/common/process_id.hpp>
#include <libs/common/types.hpp>
#include "interrupt/routing.hpp"
#include "interrupt/vector.hpp"
#include "list.hpp"
#include "task/ipc.hpp"
#include "task/message_queue.hpp"
#include "task/task.hpp"
#include "tests/framework.hpp"
#include "tests/macros.hpp"
#include "tests/test_utils.hpp"

using kernel::task::create_task;
using kernel::task::MessageQueue;
using kernel::task::notify_bit;
using kernel::task::NotifyType;
using kernel::task::Task;
using kernel::task::WaitReason;

namespace
{

/**
 * @brief Create a task parked in RUNNING so wakeups never enqueue it
 */
Task* create_parked_task(const char* name)
{
	Task* t = create_task(name, 0, true, true);
	if (t != nullptr) {
		t->state = kernel::task::TASK_RUNNING;
	}
	return t;
}

/**
 * @brief Undo a wakeup performed by a test: take the task off the run queue
 *
 * There is no list_remove, so rotate the queue once, dropping the target's
 * element and re-appending everything else in order.
 */
void remove_from_run_queue(Task* t)
{
	const size_t n = list_size(&kernel::task::run_queue);
	for (size_t i = 0; i < n; ++i) {
		list_elem_t* e = list_pop_front(&kernel::task::run_queue);
		if (e != &t->run_queue_elem) {
			list_push_back(&kernel::task::run_queue, e);
		}
	}
}

Message make_test_message(int seq)
{
	Message m = { .type = MsgType::NOTIFY_WRITE,
				  .sender = kernel::task::CURRENT_TASK->id };
	m.data.init.task_id = seq;
	return m;
}

} // namespace

void test_ipc_ring_fifo_order()
{
	Task* t = create_parked_task("ipc_fifo");
	ASSERT_NOT_NULL(t);

	for (int i = 0; i < 3; ++i) {
		Message m = make_test_message(i);
		ASSERT_EQ(kernel::task::send_message(t->id, m), OK);
	}

	Message out;
	for (int i = 0; i < 3; ++i) {
		ASSERT_TRUE(kernel::task::try_receive(t, &out));
		ASSERT_EQ(out.data.init.task_id, i);
	}
	ASSERT_FALSE(kernel::task::try_receive(t, &out));
}

void test_ipc_ring_wraparound()
{
	Task* t = create_parked_task("ipc_wrap");
	ASSERT_NOT_NULL(t);

	// Drive head_ across the ring boundary: fill half, drain half, twice
	Message out;
	int seq = 0;
	int expected = 0;
	for (int round = 0; round < 4; ++round) {
		for (size_t i = 0; i < MessageQueue::CAPACITY / 2; ++i) {
			Message m = make_test_message(seq++);
			ASSERT_EQ(kernel::task::send_message(t->id, m), OK);
		}
		for (size_t i = 0; i < MessageQueue::CAPACITY / 2; ++i) {
			ASSERT_TRUE(kernel::task::try_receive(t, &out));
			ASSERT_EQ(out.data.init.task_id, expected);
			++expected;
		}
	}
	ASSERT_TRUE(t->messages.empty());
}

void test_ipc_ring_full_rejects_then_recovers()
{
	Task* t = create_parked_task("ipc_full");
	ASSERT_NOT_NULL(t);

	Message m = make_test_message(0);
	for (size_t i = 0; i < MessageQueue::CAPACITY; ++i) {
		ASSERT_EQ(kernel::task::send_message(t->id, m), OK);
	}

	// Full ring: fail fast instead of blocking or growing (issue #314)
	ASSERT_EQ(kernel::task::send_message(t->id, m), ERR_QUEUE_FULL);

	// One slot freed -> the next send succeeds again
	Message out;
	ASSERT_TRUE(kernel::task::try_receive(t, &out));
	ASSERT_EQ(kernel::task::send_message(t->id, m), OK);

	while (kernel::task::try_receive(t, &out)) {
	}
}

void test_ipc_notify_synthesizes_message()
{
	Task* t = create_parked_task("ipc_notify");
	ASSERT_NOT_NULL(t);

	kernel::task::notify(t->id, NotifyType::TIMER);

	Message out;
	ASSERT_TRUE(kernel::task::try_receive(t, &out));
	ASSERT_TRUE(out.type == MsgType::NOTIFY_TIMER_TIMEOUT);
	ASSERT_EQ(out.sender.raw(), process_ids::INTERRUPT.raw());
	ASSERT_FALSE(kernel::task::try_receive(t, &out));
}

void test_ipc_notify_coalesces()
{
	Task* t = create_parked_task("ipc_coalesce");
	ASSERT_NOT_NULL(t);

	// Repeated doorbells collapse into one sticky bit: a burst can never
	// overflow anything, which is why ISRs may only notify (issue #314)
	kernel::task::notify(t->id, NotifyType::TIMER);
	kernel::task::notify(t->id, NotifyType::TIMER);
	kernel::task::notify(t->id, NotifyType::TIMER);

	Message out;
	ASSERT_TRUE(kernel::task::try_receive(t, &out));
	ASSERT_TRUE(out.type == MsgType::NOTIFY_TIMER_TIMEOUT);
	ASSERT_FALSE(kernel::task::try_receive(t, &out));
}

void test_ipc_notifications_drain_before_ring()
{
	Task* t = create_parked_task("ipc_order");
	ASSERT_NOT_NULL(t);

	Message m = make_test_message(7);
	ASSERT_EQ(kernel::task::send_message(t->id, m), OK);
	kernel::task::notify(t->id, NotifyType::XHCI);

	// Doorbells are delivered ahead of queued messages
	Message out;
	ASSERT_TRUE(kernel::task::try_receive(t, &out));
	ASSERT_TRUE(out.type == MsgType::NOTIFY_XHCI);
	ASSERT_TRUE(kernel::task::try_receive(t, &out));
	ASSERT_EQ(out.data.init.task_id, 7);
}

void test_ipc_notify_wakes_receive_waiter()
{
	Task* t = create_parked_task("ipc_wake_recv");
	ASSERT_NOT_NULL(t);

	t->wait_reason = WaitReason::RECEIVE;
	t->state = kernel::task::TASK_WAITING;

	kernel::task::notify(t->id, NotifyType::TIMER);

	ASSERT_EQ(t->state, kernel::task::TASK_READY);
	ASSERT_TRUE(list_contains(&kernel::task::run_queue, &t->run_queue_elem));

	remove_from_run_queue(t);
	t->state = kernel::task::TASK_RUNNING;
	Message out;
	while (kernel::task::try_receive(t, &out)) {
	}
}

void test_ipc_notify_respects_wait_mask()
{
	Task* t = create_parked_task("ipc_wake_mask");
	ASSERT_NOT_NULL(t);

	t->wait_reason = WaitReason::NOTIFY;
	t->wait_notify_mask = notify_bit(NotifyType::VIRTIO_BLK);
	t->state = kernel::task::TASK_WAITING;

	// A doorbell outside the mask must not wake the waiter
	kernel::task::notify(t->id, NotifyType::TIMER);
	ASSERT_EQ(t->state, kernel::task::TASK_WAITING);

	// The masked doorbell does
	kernel::task::notify(t->id, NotifyType::VIRTIO_BLK);
	ASSERT_EQ(t->state, kernel::task::TASK_READY);

	remove_from_run_queue(t);
	t->state = kernel::task::TASK_RUNNING;
	t->pending_notifications = 0;
	t->wait_reason = WaitReason::NONE;
}

void test_ipc_send_does_not_wake_notify_waiter()
{
	Task* t = create_parked_task("ipc_no_wake");
	ASSERT_NOT_NULL(t);

	t->wait_reason = WaitReason::NOTIFY;
	t->wait_notify_mask = notify_bit(NotifyType::VIRTIO_BLK);
	t->state = kernel::task::TASK_WAITING;

	// A device wait must not be resumed by an unrelated request: the old
	// bare sleep was, and the zeroed virtio status then read as premature
	// success (issue #314)
	Message m = make_test_message(0);
	ASSERT_EQ(kernel::task::send_message(t->id, m), OK);
	ASSERT_EQ(t->state, kernel::task::TASK_WAITING);

	t->state = kernel::task::TASK_RUNNING;
	t->wait_reason = WaitReason::NONE;
	Message out;
	while (kernel::task::try_receive(t, &out)) {
	}
}

void test_ipc_wait_notification_consumes_pending_bit()
{
	Task* t = create_parked_task("ipc_wait_bit");
	ASSERT_NOT_NULL(t);

	// Doorbell fires before the wait: wait_notification must return
	// immediately instead of sleeping (the kick-then-sleep race)
	kernel::task::notify(t->id, NotifyType::VIRTIO_BLK);

	const kernel::tests::ScopedCurrentTask scoped_task(t);
	kernel::task::wait_notification(notify_bit(NotifyType::VIRTIO_BLK));

	ASSERT_EQ(t->pending_notifications & notify_bit(NotifyType::VIRTIO_BLK), 0U);
	ASSERT_EQ(t->state, kernel::task::TASK_RUNNING);
}

void test_ipc_irq_routing_delivers_registered_doorbell()
{
	Task* t = create_parked_task("ipc_route");
	ASSERT_NOT_NULL(t);

	// Vectors nothing registers in this kernel (0x50/0x51 are unassigned).
	// Deliberately casts out-of-range values to probe unregistered vectors.
	using Number = kernel::interrupt::InterruptVector::Number;
	// NOLINTNEXTLINE(clang-analyzer-optin.core.EnumCastOutOfRange)
	const auto test_vector = static_cast<Number>(0x50);
	// NOLINTNEXTLINE(clang-analyzer-optin.core.EnumCastOutOfRange)
	const auto unrouted_vector = static_cast<Number>(0x51);

	ASSERT_EQ(kernel::interrupt::register_irq_notification(test_vector, t->id,
														   NotifyType::XHCI),
			  OK);
	kernel::interrupt::notify_irq(test_vector);

	Message out;
	ASSERT_TRUE(kernel::task::try_receive(t, &out));
	ASSERT_TRUE(out.type == MsgType::NOTIFY_XHCI);

	// An unregistered vector is counted, not delivered
	const uint64_t before = kernel::interrupt::unrouted_irq_count();
	kernel::interrupt::notify_irq(unrouted_vector);
	ASSERT_EQ(kernel::interrupt::unrouted_irq_count(), before + 1);
	ASSERT_FALSE(kernel::task::try_receive(t, &out));
}

void register_ipc_tests()
{
	test_register("ipc_ring_fifo_order", test_ipc_ring_fifo_order);
	test_register("ipc_ring_wraparound", test_ipc_ring_wraparound);
	test_register("ipc_ring_full_rejects_then_recovers",
				  test_ipc_ring_full_rejects_then_recovers);
	test_register("ipc_notify_synthesizes_message",
				  test_ipc_notify_synthesizes_message);
	test_register("ipc_notify_coalesces", test_ipc_notify_coalesces);
	test_register("ipc_notifications_drain_before_ring",
				  test_ipc_notifications_drain_before_ring);
	test_register("ipc_notify_wakes_receive_waiter",
				  test_ipc_notify_wakes_receive_waiter);
	test_register("ipc_notify_respects_wait_mask",
				  test_ipc_notify_respects_wait_mask);
	test_register("ipc_send_does_not_wake_notify_waiter",
				  test_ipc_send_does_not_wake_notify_waiter);
	test_register("ipc_wait_notification_consumes_pending_bit",
				  test_ipc_wait_notification_consumes_pending_bit);
	test_register("ipc_irq_routing_delivers_registered_doorbell",
				  test_ipc_irq_routing_delivers_registered_doorbell);
}
