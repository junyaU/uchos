#include "tests/test_cases/task_test.hpp"
#include <cstdint>
#include <cstring>
#include <libs/common/message.hpp>
#include <libs/common/process_id.hpp>
#include "task/context.hpp"
#include "task/task.hpp"
#include "tests/framework.hpp"
#include "tests/macros.hpp"

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
	ASSERT_TRUE(t->is_initilized);
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

	// Test message handler registration
	g_handler_called = false;
	t->add_msg_handler(MsgType::IPC_EXIT_TASK, test_message_handler);
	ASSERT_FALSE(g_handler_called);

	// Test message queue
	ASSERT_TRUE(t->messages.empty());
	const Message test_msg = { .type = MsgType::IPC_EXIT_TASK,
							   .sender = ProcessId::from_raw(0) };
	t->messages.push(test_msg);
	ASSERT_FALSE(t->messages.empty());

	// Test message handling
	t->message_handlers[static_cast<int32_t>(MsgType::IPC_EXIT_TASK)](test_msg);
	ASSERT_TRUE(g_handler_called);
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
	ASSERT_TRUE(t->is_initilized);

	// Test task deallocation with operator delete
	delete t;
	// Note: We can't verify the deletion directly, but the destructor should have
	// cleaned up the page tables
}

void register_task_tests()
{
	test_register("task_creation_basic", test_task_creation_basic);
	test_register("task_id_management", test_task_id_management);
	test_register("task_message_handling", test_task_message_handling);
	test_register("task_copy", test_task_copy);
	test_register("task_memory_management", test_task_memory_management);
}
