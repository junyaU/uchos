#include "tests/test_cases/task_test.hpp"
#include "task/task.hpp"
#include "tests/framework.hpp"
#include "tests/macros.hpp"
#include <libs/common/message.hpp>

void test_task_creation_basic()
{
	// Test basic task creation
	const char* task_name = "test_task";
	task* t = create_task(task_name, 0, true, true);
	ASSERT_NOT_NULL(t);

	// Verify task properties
	ASSERT_EQ(strcmp(t->name, task_name), 0);
	ASSERT_EQ(t->state, TASK_WAITING);
	ASSERT_TRUE(t->is_initilized);
	ASSERT_EQ(t->parent_id, -1);
	ASSERT_NOT_NULL(t->stack);
	ASSERT_TRUE(t->stack_size > 0);
	ASSERT_NOT_NULL(t->get_page_table());
}

void test_task_id_management()
{
	// Test getting available task ID
	pid_t id1 = get_available_task_id();
	ASSERT_TRUE(id1 >= 0);
	ASSERT_TRUE(id1 < MAX_TASKS);

	// Create first task
	task* t1 = create_task("id_test1", 0, true, true);
	ASSERT_NOT_NULL(t1);
	ASSERT_EQ(t1->id, id1);

	// Get next available ID
	pid_t id2 = get_available_task_id();
	ASSERT_TRUE(id2 >= 0);
	ASSERT_TRUE(id2 < MAX_TASKS);
	ASSERT_NE(id1, id2);

	// Create second task
	task* t2 = create_task("id_test2", 0, true, true);
	ASSERT_NOT_NULL(t2);
	ASSERT_EQ(t2->id, id2);

	// Test getting task by ID
	ASSERT_EQ(get_task(id1), t1);
	ASSERT_EQ(get_task(id2), t2);

	// Test getting task by name
	ASSERT_EQ(get_task_id_by_name("id_test1"), id1);
	ASSERT_EQ(get_task_id_by_name("id_test2"), id2);
	ASSERT_EQ(get_task_id_by_name("nonexistent"), -1);
}

namespace
{
bool g_handler_called = false;

void test_message_handler(const message&) { g_handler_called = true; }
} // namespace

void test_task_message_handling()
{
	task* t = create_task("message_test", 0, true, true);
	ASSERT_NOT_NULL(t);

	// Test message handler registration
	g_handler_called = false;
	t->add_msg_handler(msg_t::IPC_EXIT_TASK, test_message_handler);
	ASSERT_FALSE(g_handler_called);

	// Test message queue
	ASSERT_TRUE(t->messages.empty());
	message test_msg = { .type = msg_t::IPC_EXIT_TASK, .sender = 0 };
	t->messages.push(test_msg);
	ASSERT_FALSE(t->messages.empty());

	// Test message handling
	t->message_handlers[static_cast<int32_t>(msg_t::IPC_EXIT_TASK)](test_msg);
	ASSERT_TRUE(g_handler_called);
}

void test_task_copy()
{
	// Create and setup parent task
	task* parent = create_task("parent_task", 0, true, true);
	ASSERT_NOT_NULL(parent);

	// Setup parent context
	context parent_ctx = {};
	parent_ctx.rsp =
			reinterpret_cast<uint64_t>(parent->stack) + parent->stack_size - 8;
	parent_ctx.rbp = parent_ctx.rsp;
	parent_ctx.cr3 = reinterpret_cast<uint64_t>(parent->get_page_table());

	// Copy task
	task* child = copy_task(parent, &parent_ctx);
	ASSERT_NOT_NULL(child);

	// Verify child task properties
	ASSERT_EQ(child->parent_id, parent->id);
	ASSERT_TRUE(child->has_parent());
	ASSERT_NOT_NULL(child->stack);
	ASSERT_EQ(child->stack_size, parent->stack_size);
	ASSERT_NOT_NULL(child->get_page_table());
	ASSERT_NE(child->get_page_table(), parent->get_page_table());
}

void test_task_memory_management()
{
	// Test task allocation with operator new
	task* t = new task(0, "memory_test", 0, TASK_WAITING, true, true);
	ASSERT_NOT_NULL(t);

	// Verify task properties after allocation
	ASSERT_EQ(t->id, 0);
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
