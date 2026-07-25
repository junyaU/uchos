/**
 * @file tests/runner.cpp
 * @brief Staged kernel test runner implementation
 */

#include "tests/runner.hpp"
#include <array>
#include <libs/common/message.hpp>
#include <libs/common/process_id.hpp>
#include "task/ipc.hpp"
#include "task/task.hpp"
#include "tests/framework.hpp"
#include "tests/test_cases/bit_utils_test.hpp"
#include "tests/test_cases/fd_test.hpp"
#include "tests/test_cases/fs_test.hpp"
#include "tests/test_cases/graphics_test.hpp"
#include "tests/test_cases/heap_debug_test.hpp"
#include "tests/test_cases/ipc_test.hpp"
#include "tests/test_cases/memory_test.hpp"
#include "tests/test_cases/paging_test.hpp"
#include "tests/test_cases/stdio_test.hpp"
#include "tests/test_cases/task_test.hpp"
#include "tests/test_cases/timer_test.hpp"
#include "tests/test_cases/user_test.hpp"
#include "tests/test_cases/virtio_blk_test.hpp"

#ifdef KERNEL_TEST_EXIT_ENABLED
#include "tests/qemu_exit.hpp"
#endif
#ifdef KERNEL_SMOKE_TEST_ENABLED
#include "tests/smoke.hpp"
#endif

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
	run_test_suite(register_bit_utils_tests);
	run_test_suite(register_bootstrap_allocator_tests);
}

// Timer events are added and removed within the suite; whether they touch the
// slab is not yet audited, so leave the leak check off for now.
void run_timer_stage_tests()
{
	run_test_suite(register_timer_tests, /*check_leaks=*/false);
}

void run_main_stage_tests()
{
	// Leak-checked: these suites free every slab allocation they make, so
	// heap-debug builds fail them on any net growth.
	run_test_suite(register_buddy_system_tests);
	run_test_suite(register_slab_tests);
	run_test_suite(register_alloc_macro_tests);

	// Not leak-checked yet: paging retains page tables and user_copy sets up
	// user mappings that outlive the suite. Re-enable once audited (#313).
	run_test_suite(register_paging_tests, /*check_leaks=*/false);
	run_test_suite(register_user_copy_tests, /*check_leaks=*/false);

	run_test_suite(register_graphics_tests);

	snapshot_task_slots();
	// Tasks created here intentionally outlive the suite: their slots are
	// cleared (not deleted) below, so a leak check would always fire (#313).
	run_test_suite(register_task_tests, /*check_leaks=*/false);
	run_test_suite(register_ipc_tests, /*check_leaks=*/false);
	run_test_suite(register_stdio_tests, /*check_leaks=*/false);
	release_new_task_slots();

	run_test_suite(register_virtio_blk_tests);
	// FS and fd suites exercise subsystems with known leaks (#313).
	run_test_suite(register_fs_tests, /*check_leaks=*/false);
	run_test_suite(register_fd_tests, /*check_leaks=*/false);

#ifdef KERNEL_HEAP_DEBUG_ENABLED
	run_test_suite(register_heap_debug_tests);
#endif

	// The stdio suites exercise sys_write(stdout/stderr), which queues
	// NOTIFY_WRITE on the real SHELL task. Drain it so boot-time tests
	// never leak output into the interactive shell: the leak used to be
	// masked by userland's discard-on-mismatch receive loop, which is gone
	// with the correlation-matched call of issue #314 Stage B.
	kernel::task::Task* shell = kernel::task::get_task(process_ids::SHELL);
	if (shell != nullptr) {
		Message leftover;
		while (kernel::task::try_receive(shell, &leftover)) {
			// A drained message may carry an OOL payload (large sys_write);
			// undelivered means this drain owns and must free it
			kernel::task::free_message_ool(leftover);
		}
	}

	test_print_summary();

#if defined(KERNEL_SMOKE_TEST_ENABLED)
	// Boot continues into userland for the ring-3 smoke test (issue #374);
	// the QEMU exit is deferred until its report combines both results
	smoke_set_kernel_tests_result(test_all_passed());
#elif defined(KERNEL_TEST_EXIT_ENABLED)
	exit_qemu(test_all_passed());
#endif
}

} // namespace kernel::tests
