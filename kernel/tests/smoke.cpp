/**
 * @file tests/smoke.cpp
 * @brief Ring-3 fork→exec→wait smoke test implementation (issue #374)
 */

#include "tests/smoke.hpp"
#include <libs/common/message.hpp>
#include <libs/common/types.hpp>
#include "log/log.hpp"
#include "task/task.hpp"
#include "tests/qemu_exit.hpp"
#include "timers/timer.hpp"

namespace
{

// The shell needs to be loaded from disk and run one child command; on a
// TCG CI runner that is seconds, so a minute means something is stuck.
constexpr unsigned long SMOKE_WATCHDOG_MS = 60'000;

bool kernel_tests_passed = false;
bool smoke_concluded = false;

// Marker format parsed by scripts/run_kernel_tests.sh; keep the two sides
// in sync (same contract as TEST_SUMMARY).
void conclude(error_t status, const char* detail)
{
	if (smoke_concluded) {
		return;
	}
	smoke_concluded = true;

	// Strictly zero: a child exit status may arrive as a positive value,
	// which IS_OK would wave through
	const bool passed = status == OK;
	LOG_INFO("SMOKE_TEST: %s status=%d kernel_tests=%s result=%s", detail, status,
			 kernel_tests_passed ? "PASS" : "FAIL", passed ? "PASS" : "FAIL");

	kernel::tests::exit_qemu(passed && kernel_tests_passed);
}

void handle_smoke_report(const Message& m)
{
	conclude(m.data.smoke.status, "shell fork/exec/wait");
}

void handle_watchdog_timeout(const Message& m)
{
	conclude(ERR_TIMEOUT, "watchdog timeout before shell report");
}

} // namespace

namespace kernel::tests
{

void smoke_set_kernel_tests_result(bool passed) { kernel_tests_passed = passed; }

void smoke_init()
{
	kernel::task::Task* t = kernel::task::CURRENT_TASK;

	t->add_msg_handler(MsgType::SMOKE_REPORT, handle_smoke_report);
	// The KERNEL task arms no other timer, so any expiry on it is the
	// watchdog: a hang anywhere on the fork→exec→wait path still produces
	// a serial marker and a FAIL exit instead of a silent CI timeout.
	t->add_msg_handler(MsgType::NOTIFY_TIMER_TIMEOUT, handle_watchdog_timeout);
	kernel::timers::ktimer->add_timer_event(SMOKE_WATCHDOG_MS, t->id);
}

} // namespace kernel::tests
