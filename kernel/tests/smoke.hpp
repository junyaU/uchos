/**
 * @file tests/smoke.hpp
 * @brief Ring-3 fork→exec→wait smoke test for headless CI runs (issue #374)
 *
 * The in-kernel suites run entirely in ring 0, so regressions that only
 * bite on the user-mode path (fork's two returns, exec's CR3 switch,
 * sys_wait's parent/child handshake — the #371 class) pass them unseen.
 * In KERNEL_SMOKE_TEST builds the boot therefore continues past the test
 * stages into userland: the shell is exec'd with "-smoke", runs one
 * command through its normal fork→exec→wait path, and reports the outcome
 * back via a SMOKE_REPORT message. This module judges that report, prints
 * the "SMOKE_TEST: ... result=PASS|FAIL" serial marker the CI script
 * parses, and exits QEMU with the combined test + smoke result.
 */

#pragma once

struct Message;

namespace kernel::tests
{

/**
 * @brief Record the cumulative in-kernel test result for the final verdict
 *
 * Called by the test runner instead of exiting QEMU: the exit is deferred
 * until the smoke report (or the watchdog) so both results land in one run.
 *
 * @param passed Result of test_all_passed() after the main-stage suites
 */
void smoke_set_kernel_tests_result(bool passed);

/**
 * @brief Register the smoke handlers on the current (KERNEL) task and arm
 * the watchdog
 *
 * Must run on the KERNEL task after start_scheduling(), before
 * kernel_service() enters its message loop: the SMOKE_REPORT and watchdog
 * timeout handlers dispatch through that loop's handler table.
 */
void smoke_init();

} // namespace kernel::tests
