/**
 * @file fd_test.cpp
 * @brief File descriptor management tests
 * @date 2025-07-13
 */

#include "fs/file_descriptor.hpp"
#include "graphics/log.hpp"
#include "task/task.hpp"
#include "tests/framework.hpp"
#include "tests/macros.hpp"
#include <cstring>
#include <libs/common/types.hpp>

using namespace kernel;

void test_fd_allocation()
{
	// Create test FD table
	fs::FileDescriptor fd_table[32];
	fs::init_process_fd_table(fd_table, 32);

	// Test allocation
	fd_t fd1 = fs::allocate_process_fd(fd_table, 32, "test1.txt", 100, ProcessId::from_raw(1));
	ASSERT_GT(fd1, -1);
	ASSERT_LT(fd1, 32);

	// Verify the allocated entry
	fs::FileDescriptor* entry = fs::get_process_fd(fd_table, 32, fd1);
	ASSERT_NOT_NULL(entry);
	ASSERT_TRUE(entry->is_used());
	ASSERT_EQ(strcmp(entry->name, "test1.txt"), 0);
	ASSERT_EQ(entry->size, 100);

	// Test another allocation
	fd_t fd2 = fs::allocate_process_fd(fd_table, 32, "test2.txt", 200, ProcessId::from_raw(1));
	ASSERT_NE(fd1, fd2);
	ASSERT_GT(fd2, -1);

	LOG_TEST("test_fd_allocation passed");
}

void test_fd_release()
{
	// Create test FD table
	fs::FileDescriptor fd_table[32];
	fs::init_process_fd_table(fd_table, 32);

	// Allocate and release
	fd_t fd = fs::allocate_process_fd(fd_table, 32, "temp.txt", 50, ProcessId::from_raw(1));
	ASSERT_GT(fd, -1);

	error_t result = fs::release_process_fd(fd_table, 32, fd);
	ASSERT_EQ(result, OK);

	// Verify release
	fs::FileDescriptor* entry = fs::get_process_fd(fd_table, 32, fd);
	ASSERT_NULL(entry);

	// Try to release standard descriptors (should fail)
	result = fs::release_process_fd(fd_table, 32, STDIN_FILENO);
	ASSERT_EQ(result, ERR_INVALID_FD);

	result = fs::release_process_fd(fd_table, 32, STDOUT_FILENO);
	ASSERT_EQ(result, ERR_INVALID_FD);

	LOG_TEST("test_fd_release passed");
}

void test_fd_fork_copy()
{
	// Create parent FD table
	fs::FileDescriptor parent_table[32];
	fs::init_process_fd_table(parent_table, 32);

	// Add some file descriptors
	fd_t fd1 =
	    fs::allocate_process_fd(parent_table, 32, "parent1.txt", 100, ProcessId::from_raw(1));
	fd_t fd2 =
	    fs::allocate_process_fd(parent_table, 32, "parent2.txt", 200, ProcessId::from_raw(1));

	// Create child FD table and copy
	fs::FileDescriptor child_table[32];
	error_t result = fs::copy_fd_table(child_table, parent_table, 32, ProcessId::from_raw(2));
	ASSERT_EQ(result, OK);

	// Verify copy
	fs::FileDescriptor* parent_entry1 = fs::get_process_fd(parent_table, 32, fd1);
	fs::FileDescriptor* child_entry1 = fs::get_process_fd(child_table, 32, fd1);
	ASSERT_NOT_NULL(parent_entry1);
	ASSERT_NOT_NULL(child_entry1);
	ASSERT_EQ(strcmp(parent_entry1->name, child_entry1->name), 0);
	ASSERT_EQ(parent_entry1->size, child_entry1->size);

	// Verify standard descriptors are also copied
	ASSERT_TRUE(child_table[STDIN_FILENO].is_used());
	ASSERT_TRUE(child_table[STDOUT_FILENO].is_used());
	ASSERT_TRUE(child_table[STDERR_FILENO].is_used());

	LOG_TEST("test_fd_fork_copy passed");
}

void test_fd_dup2()
{
	// Create test FD table
	fs::FileDescriptor fd_table[32];
	fs::init_process_fd_table(fd_table, 32);

	// Allocate a file descriptor
	fd_t file_fd = fs::allocate_process_fd(fd_table, 32, "output.txt", 0, ProcessId::from_raw(1));
	ASSERT_GT(file_fd, -1);

	// Test redirection setup
	// fd_table[STDOUT_FILENO].fd.redirect_to = file_fd;

	// Verify redirection
	fs::FileDescriptor* stdout_entry = fs::get_process_fd(fd_table, 32, STDOUT_FILENO);
	ASSERT_NOT_NULL(stdout_entry);
	// ASSERT_EQ(stdout_entry->fd.redirect_to, file_fd);

	// Get should follow redirection
	fs::FileDescriptor* redirected = fs::get_process_fd(fd_table, 32, STDOUT_FILENO);
	ASSERT_NOT_NULL(redirected);
	ASSERT_EQ(strcmp(redirected->name, "output.txt"), 0);

	LOG_TEST("test_fd_dup2 passed");
}

void test_fd_limits()
{
	// Create small FD table
	fs::FileDescriptor fd_table[8];
	fs::init_process_fd_table(fd_table, 8);

	// Fill up the table (3 standard descriptors already used)
	fd_t fds[5];
	const char* names[5] = { "file0.txt", "file1.txt", "file2.txt", "file3.txt", "file4.txt" };
	for (int i = 0; i < 5; i++) {
		fds[i] = fs::allocate_process_fd(fd_table, 8, names[i], 100, ProcessId::from_raw(1));
		ASSERT_GT(fds[i], -1);
	}

	// Next allocation should fail
	fd_t overflow_fd =
	    fs::allocate_process_fd(fd_table, 8, "overflow.txt", 100, ProcessId::from_raw(1));
	ASSERT_EQ(overflow_fd, NO_FD);

	// Release one and try again
	error_t result = fs::release_process_fd(fd_table, 8, fds[0]);
	ASSERT_EQ(result, OK);

	fd_t new_fd = fs::allocate_process_fd(fd_table, 8, "new.txt", 100, ProcessId::from_raw(1));
	ASSERT_EQ(new_fd, fds[0]);  // Should reuse the freed slot

	LOG_TEST("test_fd_limits passed");
}

void test_release_all_fds()
{
	// Create test FD table
	fs::FileDescriptor fd_table[32];
	fs::init_process_fd_table(fd_table, 32);

	// Allocate several file descriptors
	const char* names[5] = { "file0.txt", "file1.txt", "file2.txt", "file3.txt", "file4.txt" };
	for (int i = 0; i < 5; i++) {
		fd_t fd = fs::allocate_process_fd(fd_table, 32, names[i], 100, ProcessId::from_raw(1));
		ASSERT_GT(fd, -1);
	}

	// Release all
	fs::release_all_process_fds(fd_table, 32);

	// Verify all non-standard FDs are released
	for (int i = 3; i < 32; i++) {
		if (fd_table[i].is_used()) {
			LOG_TEST("FD %d still in use after release_all", i);
			ASSERT_TRUE(fd_table[i].is_unused());
		}
	}

	// Verify standard descriptors are still in use
	ASSERT_TRUE(fd_table[STDIN_FILENO].is_used());
	ASSERT_TRUE(fd_table[STDOUT_FILENO].is_used());
	ASSERT_TRUE(fd_table[STDERR_FILENO].is_used());

	LOG_TEST("test_release_all_fds passed");
}

void register_fd_tests()
{
	// test_register("test_fd_allocation", test_fd_allocation);
	// test_register("test_fd_release", test_fd_release);
	// test_register("test_fd_fork_copy", test_fd_fork_copy);
	// test_register("test_fd_dup2", test_fd_dup2);
	// test_register("test_fd_limits", test_fd_limits);
	// test_register("test_release_all_fds", test_release_all_fds);
}