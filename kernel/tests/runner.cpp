/**
 * @file tests/runner.cpp
 * @brief Staged kernel test runner implementation
 */

#include "tests/runner.hpp"
#include <array>
#include "task/task.hpp"
#include "tests/framework.hpp"
#include "tests/test_cases/fd_test.hpp"
#include "tests/test_cases/fs_test.hpp"
#include "tests/test_cases/memory_test.hpp"
#include "tests/test_cases/stdio_test.hpp"
#include "tests/test_cases/task_test.hpp"
#include "tests/test_cases/timer_test.hpp"
#include "tests/test_cases/virtio_blk_test.hpp"

namespace
{

std::array<bool, kernel::task::MAX_TASKS> slot_snapshot;

void snapshot_task_slots()
{
	for (int i = 0; i < kernel::task::MAX_TASKS; ++i) {
		slot_snapshot[i] = kernel::task::tasks[i] != nullptr;
	}
}

// Tasks created by test cases are never scheduled, so clearing the slot is
// enough to keep the task table usable after the suites. The Task objects
// themselves are not deleted here: ~Task frees the page tables, which are
// partly shared with the kernel address space (see #313).
void release_new_task_slots()
{
	for (int i = 0; i < kernel::task::MAX_TASKS; ++i) {
		if (!slot_snapshot[i] && kernel::task::tasks[i] != nullptr) {
			kernel::task::tasks[i] = nullptr;
		}
	}
}

} // namespace

namespace kernel::tests
{

void run_bootstrap_stage_tests()
{
	run_test_suite(register_bootstrap_allocator_tests);
}

void run_timer_stage_tests() { run_test_suite(register_timer_tests); }

void run_main_stage_tests()
{
	run_test_suite(register_buddy_system_tests);
	run_test_suite(register_slab_tests);
	run_test_suite(register_alloc_macro_tests);

	snapshot_task_slots();
	run_test_suite(register_task_tests);
	run_test_suite(register_stdio_tests);
	release_new_task_slots();

	run_test_suite(register_virtio_blk_tests);
	run_test_suite(register_fs_tests);
	run_test_suite(register_fd_tests);

	test_print_summary();
}

} // namespace kernel::tests
