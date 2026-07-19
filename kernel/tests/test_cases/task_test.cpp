#include "tests/test_cases/task_test.hpp"
#include <cstdint>
#include <cstring>
#include <libs/common/message.hpp>
#include <libs/common/process_id.hpp>
#include "memory/slab.hpp"
#include "task/context.hpp"
#include "task/ipc.hpp"
#include "task/task.hpp"
#include "tests/framework.hpp"
#include "tests/macros.hpp"
#include "tests/test_utils.hpp"

// Forward declaration for the syscall under test
namespace kernel::syscall
{
extern error_t sys_ipc(uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t arg4);
} // namespace kernel::syscall

using kernel::task::create_task;
using kernel::task::get_available_task_id;
using kernel::task::get_task;
using kernel::task::get_task_id_by_name;
using kernel::task::MAX_TASKS;
using kernel::task::Task;
using kernel::task::TASK_WAITING;

void test_task_creation_basic()
{
	// Test basic task creation
	const char* task_name = "test_task";
	Task* t = create_task(task_name, 0, true, true);
	ASSERT_NOT_NULL(t);

	// Verify task properties
	ASSERT_EQ(strcmp(t->name, task_name), 0);
	ASSERT_EQ(t->state, TASK_WAITING);
	ASSERT_TRUE(t->is_initialized);
	ASSERT_EQ(t->parent_id.raw(), -1);
	ASSERT_NOT_NULL(t->stack);
	ASSERT_TRUE(t->stack_size > 0);
	ASSERT_NOT_NULL(t->get_page_table());
}

void test_task_id_management()
{
	// Test getting available task ID
	const ProcessId id1 = get_available_task_id();
	ASSERT_TRUE(id1.raw() >= 0);
	ASSERT_TRUE(id1.raw() < MAX_TASKS);

	// Create first task
	Task* t1 = create_task("id_test1", 0, true, true);
	ASSERT_NOT_NULL(t1);
	ASSERT_TRUE(t1->id == id1);

	// Get next available ID
	const ProcessId id2 = get_available_task_id();
	ASSERT_TRUE(id2.raw() >= 0);
	ASSERT_TRUE(id2.raw() < MAX_TASKS);
	ASSERT_FALSE(id1 == id2);

	// Create second task
	Task* t2 = create_task("id_test2", 0, true, true);
	ASSERT_NOT_NULL(t2);
	ASSERT_TRUE(t2->id == id2);

	// Test getting task by ID
	ASSERT_EQ(get_task(id1), t1);
	ASSERT_EQ(get_task(id2), t2);

	// Test getting task by name
	ASSERT_TRUE(get_task_id_by_name("id_test1") == id1);
	ASSERT_TRUE(get_task_id_by_name("id_test2") == id2);
	ASSERT_EQ(get_task_id_by_name("nonexistent").raw(), -1);
}

namespace
{
bool g_handler_called = false;

void test_message_handler(const Message& /*unused*/) { g_handler_called = true; }
} // namespace

void test_task_message_handling()
{
	Task* t = create_task("message_test", 0, true, true);
	ASSERT_NOT_NULL(t);

	// Keep the receiver off the run queue while the test feeds it
	t->state = kernel::task::TASK_RUNNING;

	// Test message handler registration
	g_handler_called = false;
	t->add_msg_handler(MsgType::IPC_EXIT_TASK, test_message_handler);
	ASSERT_FALSE(g_handler_called);

	// The ring is sealed against direct pushes; messages enter through
	// send_message only (issue #314)
	ASSERT_TRUE(t->messages.empty());
	Message test_msg = { .type = MsgType::IPC_EXIT_TASK,
						 .sender = ProcessId::from_raw(0) };
	ASSERT_EQ(kernel::task::send_message(t->id, test_msg), OK);
	ASSERT_FALSE(t->messages.empty());

	// Test message handling through the shared dispatch point
	Message received;
	ASSERT_TRUE(kernel::task::try_receive(t, &received));
	t->dispatch_message(received);
	ASSERT_TRUE(g_handler_called);
	ASSERT_TRUE(t->messages.empty());
}

void test_task_copy()
{
	// Create and setup parent task
	Task* parent = create_task("parent_task", 0, true, true);
	ASSERT_NOT_NULL(parent);

	// Setup parent context
	kernel::task::Context parent_ctx = {};
	parent_ctx.rsp =
			reinterpret_cast<uint64_t>(parent->stack) + parent->stack_size - 8;
	parent_ctx.rbp = parent_ctx.rsp;
	parent_ctx.cr3 = reinterpret_cast<uint64_t>(parent->get_page_table());

	// Copy task
	Task* child = kernel::task::copy_task(parent, &parent_ctx);
	ASSERT_NOT_NULL(child);

	// Verify child task properties
	ASSERT_TRUE(child->parent_id == parent->id);
	ASSERT_TRUE(child->has_parent());
	ASSERT_NOT_NULL(child->stack);
	ASSERT_EQ(child->stack_size, parent->stack_size);
	ASSERT_NOT_NULL(child->get_page_table());
	ASSERT_NE(child->get_page_table(), parent->get_page_table());
}

void test_task_memory_management()
{
	// Test task allocation with operator new
	Task* t = new Task(0, "memory_test", 0, TASK_WAITING, true, true);
	ASSERT_NOT_NULL(t);

	// Verify task properties after allocation
	ASSERT_EQ(t->id.raw(), 0);
	ASSERT_EQ(strcmp(t->name, "memory_test"), 0);
	ASSERT_EQ(t->state, TASK_WAITING);
	ASSERT_TRUE(t->is_initialized);

	void* stack = t->stack;
	ASSERT_NOT_NULL(stack);
	ASSERT_TRUE(kernel::memory::is_slab_object_in_use(stack));

	// Test task deallocation with operator delete
	delete t;

	// ~Task must free the kernel stack along with the page tables (issue #313).
	// Deliberately probes a freed pointer; see ExpectedViolation-style tests
	// in memory_test.cpp for the same pattern.
	// NOLINTNEXTLINE(clang-analyzer-unix.Malloc)
	ASSERT_FALSE(kernel::memory::is_slab_object_in_use(stack));
}

void test_wait_for_message_preserves_other_messages()
{
	Task* t = create_task("wait_msg_test", 0, true, true);
	ASSERT_NOT_NULL(t);

	t->state = kernel::task::TASK_RUNNING;

	Message other = { .type = MsgType::NOTIFY_WRITE,
					  .sender = ProcessId::from_raw(1) };
	Message target = { .type = MsgType::IPC_EXIT_TASK,
					   .sender = ProcessId::from_raw(2) };
	ASSERT_EQ(kernel::task::send_message(t->id, other), OK);
	ASSERT_EQ(kernel::task::send_message(t->id, target), OK);

	const kernel::tests::ScopedCurrentTask scoped_task(t);

	// The matching message is extracted even when it is not at the front
	const Message m = kernel::task::wait_for_message(MsgType::IPC_EXIT_TASK);
	ASSERT_TRUE(m.type == MsgType::IPC_EXIT_TASK);
	ASSERT_EQ(m.sender.raw(), 2);

	// The unrelated message is still queued (issue #313: the old
	// implementation spun on and reordered non-matching messages)
	ASSERT_EQ(t->messages.size(), 1UL);
	Message remaining;
	ASSERT_TRUE(kernel::task::try_receive(t, &remaining));
	ASSERT_TRUE(remaining.type == MsgType::NOTIFY_WRITE);
}

void test_send_message_queue_cap()
{
	Task* t = create_task("queue_cap_test", 0, true, true);
	ASSERT_NOT_NULL(t);

	// Keep the receiver off the run queue while flooding it
	t->state = kernel::task::TASK_RUNNING;

	Message m = { .type = MsgType::NOTIFY_WRITE,
				  .sender = kernel::task::CURRENT_TASK->id };

	// The return value is reliable now that the ISR-only attribute moved
	// to notify() (issue #314)
	for (size_t i = 0; i < kernel::task::MessageQueue::CAPACITY; ++i) {
		ASSERT_EQ(kernel::task::send_message(t->id, m), OK);
	}

	// The ring rejects the overflowing message instead of growing (the
	// backpressure decision of issue #314: fail fast, never block)
	ASSERT_EQ(kernel::task::send_message(t->id, m), ERR_QUEUE_FULL);
	ASSERT_EQ(t->messages.size(), kernel::task::MessageQueue::CAPACITY);

	Message drained;
	while (kernel::task::try_receive(t, &drained)) {
	}
	ASSERT_TRUE(t->messages.empty());
}

void test_ipc_recv_returns_queued_message()
{
	Task* t = create_task("ipc_recv_test", 0, true, true);
	ASSERT_NOT_NULL(t);
	t->state = kernel::task::TASK_RUNNING;

	// sys_ipc(IPC_RECV) blocks on an empty queue now (issue #314), so the
	// single-threaded test feeds the queue first and asserts the delivery
	Message queued = { .type = MsgType::NOTIFY_WRITE,
					   .sender = kernel::task::CURRENT_TASK->id };
	ASSERT_EQ(kernel::task::send_message(t->id, queued), OK);

	const kernel::tests::ScopedCurrentTask scoped_task(t);

	// The out-pointer is a kernel address here, so copy_to_user rejects it
	// after the receive; draining the ring below still proves the blocking
	// path popped the message. Capture the result instead of calling
	// sys_ipc inside an ASSERT: the macros re-evaluate their arguments on
	// failure, and a second IPC_RECV would block forever on the empty ring.
	Message m;
	const error_t err =
			kernel::syscall::sys_ipc(0, 0, reinterpret_cast<uint64_t>(&m), IPC_RECV);
	ASSERT_EQ(err, ERR_INVALID_ARG);
	ASSERT_TRUE(t->messages.empty());

	// The task must return to the caller RUNNING, never WAITING; it would
	// be dropped from the run queue on the next preemption (issue #313)
	ASSERT_EQ(t->state, kernel::task::TASK_RUNNING);
}

void register_task_tests()
{
	test_register("task_creation_basic", test_task_creation_basic);
	test_register("task_id_management", test_task_id_management);
	test_register("task_message_handling", test_task_message_handling);
	test_register("task_copy", test_task_copy);
	test_register("task_memory_management", test_task_memory_management);
	test_register("wait_for_message_preserves_other_messages",
				  test_wait_for_message_preserves_other_messages);
	test_register("send_message_queue_cap", test_send_message_queue_cap);
	test_register("ipc_recv_returns_queued_message",
				  test_ipc_recv_returns_queued_message);
}
