/**
 * @file tests/runner.cpp
 * @brief Staged kernel test runner implementation
 */

#include "tests/runner.hpp"
#include <array>
#include "task/task.hpp"
#include "tests/framework.hpp"
#include "tests/test_cases/bit_utils_test.hpp"
#include "tests/test_cases/fd_test.hpp"
#include "tests/test_cases/fs_test.hpp"
#include "tests/test_cases/graphics_test.hpp"
#include "tests/test_cases/memory_test.hpp"
#include "tests/test_cases/paging_test.hpp"
#include "tests/test_cases/stdio_test.hpp"
#include "tests/test_cases/task_test.hpp"
#include "tests/test_cases/timer_test.hpp"
#include "tests/test_cases/user_test.hpp"
#include "tests/test_cases/virtio_blk_test.hpp"

#ifdef KERNEL_TEST_EXIT_ENABLED
#include "asm_utils.h"
#endif

namespace
{

#ifdef KERNEL_TEST_EXIT_ENABLED
constexpr uint16_t QEMU_ISA_DEBUG_EXIT_PORT = 0xf4;
constexpr uint8_t QEMU_EXIT_CODE_PASS = 0x10; // QEMU exits with (0x10 << 1) | 1 = 33
constexpr uint8_t QEMU_EXIT_CODE_FAIL = 0x11; // QEMU exits with (0x11 << 1) | 1 = 35

// Writes the test result to the isa-debug-exit device so QEMU terminates
// with a status the CI runner can check. When the device is absent
// (interactive run), the write is ignored and boot continues normally.
void exit_qemu(bool passed)
{
	write_to_io_port8(QEMU_ISA_DEBUG_EXIT_PORT,
					   passed ? QEMU_EXIT_CODE_PASS : QEMU_EXIT_CODE_FAIL);
}
#endif

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
	run_test_suite(register_bit_utils_tests);
	run_test_suite(register_bootstrap_allocator_tests);
}

void run_timer_stage_tests() { run_test_suite(register_timer_tests); }

void run_main_stage_tests()
{
	run_test_suite(register_buddy_system_tests);
	run_test_suite(register_slab_tests);
	run_test_suite(register_alloc_macro_tests);
	run_test_suite(register_paging_tests);
	run_test_suite(register_user_copy_tests);
	run_test_suite(register_graphics_tests);

	snapshot_task_slots();
	run_test_suite(register_task_tests);
	run_test_suite(register_stdio_tests);
	release_new_task_slots();

	run_test_suite(register_virtio_blk_tests);
	run_test_suite(register_fs_tests);
	run_test_suite(register_fd_tests);

	test_print_summary();

#ifdef KERNEL_TEST_EXIT_ENABLED
	exit_qemu(test_all_passed());
#endif
}

} // namespace kernel::tests
