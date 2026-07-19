/**
 * @file tests/test_cases/ipc_test.cpp
 * @brief Tests for the IPC v2 primitives (issue #314): fixed-capacity
 * message ring, doorbell notifications, wakeup gating, the IRQ routing
 * table, reply-slot delivery (correlation matching) and child-exit records.
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

// Forward declaration for the syscall under test
namespace kernel::syscall
{
extern ProcessId sys_wait(uint64_t arg1);
} // namespace kernel::syscall

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

	// Vectors nothing registers in this kernel (0x50/0x51 are unassigned)
	const auto test_vector =
			static_cast<kernel::interrupt::InterruptVector::Number>(0x50);
	const auto unrouted_vector =
			static_cast<kernel::interrupt::InterruptVector::Number>(0x51);

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

void test_ipc_reply_delivered_to_slot()
{
	Task* caller = create_parked_task("ipc_reply_slot");
	ASSERT_NOT_NULL(caller);

	// Simulate a task blocked in call(): outstanding correlation + REPLY wait
	caller->call_correlation = 42;
	caller->reply_pending = false;
	caller->wait_reason = WaitReason::REPLY;
	caller->state = kernel::task::TASK_WAITING;

	Message req = { .type = MsgType::IPC_MEMORY_USAGE, .sender = caller->id };
	req.correlation = 42;

	Message resp = { .type = MsgType::IPC_MEMORY_USAGE,
					 .sender = kernel::task::CURRENT_TASK->id };
	resp.result = OK;
	ASSERT_EQ(kernel::task::reply(req, &resp), OK);

	// Delivered to the slot (not the ring), correlation echoed, caller woken
	ASSERT_TRUE(caller->reply_pending);
	ASSERT_TRUE(caller->messages.empty());
	ASSERT_EQ(caller->reply_slot.correlation, 42U);
	ASSERT_EQ(caller->reply_slot.flags & MSG_FLAG_REPLY, MSG_FLAG_REPLY);
	ASSERT_EQ(caller->state, kernel::task::TASK_READY);

	remove_from_run_queue(caller);
	caller->state = kernel::task::TASK_RUNNING;
	caller->call_correlation = 0;
	caller->reply_pending = false;
	caller->wait_reason = WaitReason::NONE;
}

void test_ipc_reply_noop_for_fire_and_forget()
{
	Task* t = create_parked_task("ipc_reply_noop");
	ASSERT_NOT_NULL(t);

	// A request with correlation 0 expects no reply: reply() is a no-op
	Message req = { .type = MsgType::FS_CLOSE, .sender = t->id };
	req.correlation = 0;

	Message resp = { .type = MsgType::FS_CLOSE,
					 .sender = kernel::task::CURRENT_TASK->id };
	ASSERT_EQ(kernel::task::reply(req, &resp), OK);

	ASSERT_FALSE(t->reply_pending);
	ASSERT_TRUE(t->messages.empty());
}

void test_ipc_stale_reply_dropped()
{
	Task* t = create_parked_task("ipc_stale");
	ASSERT_NOT_NULL(t);

	// No outstanding call: a reply-flagged message must be dropped with an
	// error, never queued (a late reply is a protocol bug made visible)
	Message stale = { .type = MsgType::FS_READ,
					  .sender = kernel::task::CURRENT_TASK->id };
	stale.correlation = 7;
	stale.flags = MSG_FLAG_REPLY;

	ASSERT_EQ(kernel::task::send_message(t->id, stale), ERR_INVALID_ARG);
	ASSERT_TRUE(t->messages.empty());
	ASSERT_FALSE(t->reply_pending);

	// Wrong correlation while a call is outstanding: also dropped
	t->call_correlation = 9;
	ASSERT_EQ(kernel::task::send_message(t->id, stale), ERR_INVALID_ARG);
	ASSERT_FALSE(t->reply_pending);
	t->call_correlation = 0;
}

void test_ipc_send_does_not_wake_reply_waiter()
{
	Task* caller = create_parked_task("ipc_reply_gate");
	ASSERT_NOT_NULL(caller);

	caller->call_correlation = 5;
	caller->reply_pending = false;
	caller->wait_reason = WaitReason::REPLY;
	caller->state = kernel::task::TASK_WAITING;

	// An ordinary message queues without waking the REPLY waiter; it is
	// handled after the call completes (natural server serialization)
	Message m = make_test_message(1);
	ASSERT_EQ(kernel::task::send_message(caller->id, m), OK);
	ASSERT_EQ(caller->state, kernel::task::TASK_WAITING);
	ASSERT_EQ(caller->messages.size(), 1UL);

	// The matching reply is what wakes it
	Message req = { .type = MsgType::FS_READ, .sender = caller->id };
	req.correlation = 5;
	Message resp = { .type = MsgType::FS_READ,
					 .sender = kernel::task::CURRENT_TASK->id };
	ASSERT_EQ(kernel::task::reply(req, &resp), OK);
	ASSERT_EQ(caller->state, kernel::task::TASK_READY);
	ASSERT_TRUE(caller->reply_pending);
	ASSERT_EQ(caller->messages.size(), 1UL);

	remove_from_run_queue(caller);
	caller->state = kernel::task::TASK_RUNNING;
	caller->call_correlation = 0;
	caller->reply_pending = false;
	caller->wait_reason = WaitReason::NONE;
	Message drained;
	while (kernel::task::try_receive(caller, &drained)) {
	}
}

void test_ipc_child_exit_records()
{
	Task* parent = create_parked_task("ipc_child_rec");
	ASSERT_NOT_NULL(parent);

	// Records accumulate FIFO while the parent is not waiting
	kernel::task::record_child_exit(parent, ProcessId::from_raw(90), 3);
	kernel::task::record_child_exit(parent, ProcessId::from_raw(91), 0);
	ASSERT_EQ(parent->num_exit_records, 2);
	ASSERT_EQ(parent->exit_records[0].pid, 90);
	ASSERT_EQ(parent->exit_records[0].status, 3);
	ASSERT_EQ(parent->exit_records[1].pid, 91);

	// A CHILD waiter is woken by a new record
	parent->num_exit_records = 0;
	parent->wait_reason = WaitReason::CHILD;
	parent->state = kernel::task::TASK_WAITING;
	kernel::task::record_child_exit(parent, ProcessId::from_raw(92), 1);
	ASSERT_EQ(parent->state, kernel::task::TASK_READY);
	ASSERT_EQ(parent->num_exit_records, 1);

	remove_from_run_queue(parent);
	parent->state = kernel::task::TASK_RUNNING;
	parent->wait_reason = WaitReason::NONE;
	parent->num_exit_records = 0;
}

void test_ipc_sys_wait_pops_record()
{
	Task* parent = create_parked_task("ipc_sys_wait");
	ASSERT_NOT_NULL(parent);

	// Pre-fill the record BEFORE calling sys_wait: with none present the
	// call would block forever in this single-threaded test
	kernel::task::record_child_exit(parent, ProcessId::from_raw(93), 0);

	const kernel::tests::ScopedCurrentTask scoped_task(parent);

	// The status out-pointer is a kernel address, so the copy_to_user at
	// the end fails and the pid comes back -1; popping the record and
	// staying RUNNING are what this asserts. Captured first: the assert
	// macros re-evaluate on failure and a second sys_wait would block.
	int status = 0;
	const ProcessId pid =
			kernel::syscall::sys_wait(reinterpret_cast<uint64_t>(&status));
	(void)pid;

	ASSERT_EQ(parent->num_exit_records, 0);
	ASSERT_EQ(parent->state, kernel::task::TASK_RUNNING);
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
	test_register("ipc_reply_delivered_to_slot", test_ipc_reply_delivered_to_slot);
	test_register("ipc_reply_noop_for_fire_and_forget",
				  test_ipc_reply_noop_for_fire_and_forget);
	test_register("ipc_stale_reply_dropped", test_ipc_stale_reply_dropped);
	test_register("ipc_send_does_not_wake_reply_waiter",
				  test_ipc_send_does_not_wake_reply_waiter);
	test_register("ipc_child_exit_records", test_ipc_child_exit_records);
	test_register("ipc_sys_wait_pops_record", test_ipc_sys_wait_pops_record);
}
