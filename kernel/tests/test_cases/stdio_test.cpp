#include "tests/test_cases/stdio_test.hpp"
#include "task/context.hpp"
#include "task/task.hpp"
#include "tests/framework.hpp"
#include "tests/macros.hpp"
#include <cstdint>
#include <cstring>
#include <libs/common/types.hpp>
#include <sys/types.h>

using kernel::task::create_task;
using kernel::task::CURRENT_TASK;
using kernel::task::MAX_FDS_PER_PROCESS;
using kernel::task::task;

// Forward declarations for syscalls
namespace kernel::syscall
{
extern ssize_t sys_read(uint64_t arg1, uint64_t arg2, uint64_t arg3);
extern ssize_t sys_write(uint64_t arg1, uint64_t arg2, uint64_t arg3);
} // namespace kernel::syscall

void test_fd_table_init()
{
	// Create a test task
	task* t = create_task("fd_test", 0, true, true);
	ASSERT_NOT_NULL(t);

	// Check standard file descriptors are initialized
	ASSERT_EQ(t->fd_table[STDIN_FILENO], STDIN_FILENO);
	ASSERT_EQ(t->fd_table[STDOUT_FILENO], STDOUT_FILENO);
	ASSERT_EQ(t->fd_table[STDERR_FILENO], STDERR_FILENO);

	// Check other FDs are set to NO_FD
	for (int i = 3; i < MAX_FDS_PER_PROCESS; ++i) {
		ASSERT_EQ(t->fd_table[i], NO_FD);
	}
}

void test_write_to_stdout()
{
	// Create a test task and set it as current
	task* t = create_task("write_test", 0, true, true);
	ASSERT_NOT_NULL(t);

	task* prev_task = CURRENT_TASK;
	CURRENT_TASK = t;

	// Test writing to stdout
	const char* test_msg = "Hello, stdout!";
	const ssize_t result = kernel::syscall::sys_write(
			STDOUT_FILENO, reinterpret_cast<uint64_t>(test_msg), strlen(test_msg));

	// Should return number of bytes written
	ASSERT_GT(result, 0);
	ASSERT_EQ(result, strlen(test_msg));

	// Restore previous task
	CURRENT_TASK = prev_task;
}

void test_write_to_stderr()
{
	// Create a test task and set it as current
	task* t = create_task("stderr_test", 0, true, true);
	ASSERT_NOT_NULL(t);

	task* prev_task = CURRENT_TASK;
	CURRENT_TASK = t;

	// Test writing to stderr
	const char* test_msg = "Error message";
	const ssize_t result = kernel::syscall::sys_write(
			STDERR_FILENO, reinterpret_cast<uint64_t>(test_msg), strlen(test_msg));

	// Should return number of bytes written
	ASSERT_GT(result, 0);
	ASSERT_EQ(result, strlen(test_msg));

	// Restore previous task
	CURRENT_TASK = prev_task;
}

void test_read_from_stdin()
{
	// Create a test task and set it as current
	task* t = create_task("read_test", 0, true, true);
	ASSERT_NOT_NULL(t);

	task* prev_task = CURRENT_TASK;
	CURRENT_TASK = t;

	// Test reading from stdin (currently returns 0)
	char buffer[128];
	const ssize_t result = kernel::syscall::sys_read(
			STDIN_FILENO, reinterpret_cast<uint64_t>(buffer), sizeof(buffer));

	// Currently stdin returns 0 (no data available)
	ASSERT_EQ(result, 0);

	// Restore previous task
	CURRENT_TASK = prev_task;
}

void test_invalid_fd()
{
	// Create a test task and set it as current
	task* t = create_task("invalid_fd_test", 0, true, true);
	ASSERT_NOT_NULL(t);

	task* prev_task = CURRENT_TASK;
	CURRENT_TASK = t;

	// Test invalid file descriptor (negative)
	char buffer[128];
	const ssize_t result = kernel::syscall::sys_write(
			-1, reinterpret_cast<uint64_t>(buffer), sizeof(buffer));
	ASSERT_EQ(result, ERR_INVALID_FD);

	// Test invalid file descriptor (too large)
	const ssize_t result2 = kernel::syscall::sys_write(
			MAX_FDS_PER_PROCESS + 1, reinterpret_cast<uint64_t>(buffer),
			sizeof(buffer));
	ASSERT_EQ(result2, ERR_INVALID_FD);

	// Test uninitialized file descriptor
	const ssize_t result3 = kernel::syscall::sys_write(
			10, reinterpret_cast<uint64_t>(buffer), sizeof(buffer));
	ASSERT_EQ(result3, ERR_INVALID_FD);

	// Restore previous task
	CURRENT_TASK = prev_task;
}

void test_fd_inheritance()
{
	// Create parent task
	task* parent = create_task("parent_task", 0, true, true);
	ASSERT_NOT_NULL(parent);

	// Modify parent's FD table
	parent->fd_table[5] = 5; // Simulate an open file

	// Set parent as current task for copy_task
	task* prev_task = CURRENT_TASK;
	CURRENT_TASK = parent;

	// Create child task (this would normally be done by fork)
	kernel::task::context dummy_ctx = {};
	task* child = kernel::task::copy_task(parent, &dummy_ctx);
	ASSERT_NOT_NULL(child);

	// Check that child inherited parent's FD table
	ASSERT_EQ(child->fd_table[STDIN_FILENO], parent->fd_table[STDIN_FILENO]);
	ASSERT_EQ(child->fd_table[STDOUT_FILENO], parent->fd_table[STDOUT_FILENO]);
	ASSERT_EQ(child->fd_table[STDERR_FILENO], parent->fd_table[STDERR_FILENO]);
	ASSERT_EQ(child->fd_table[5], parent->fd_table[5]);

	// Restore previous task
	CURRENT_TASK = prev_task;
}

void register_stdio_tests()
{
	test_register("test_fd_table_init", test_fd_table_init);
	test_register("test_write_to_stdout", test_write_to_stdout);
	test_register("test_write_to_stderr", test_write_to_stderr);
	test_register("test_read_from_stdin", test_read_from_stdin);
	test_register("test_invalid_fd", test_invalid_fd);
	test_register("test_fd_inheritance", test_fd_inheritance);
}
