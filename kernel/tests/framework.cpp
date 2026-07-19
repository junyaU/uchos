#include "tests/framework.hpp"
#include <cstring>
#include "log/log.hpp"
#ifdef KERNEL_HEAP_DEBUG_ENABLED
#include "memory/heap_debug.hpp"
#endif

struct TestCaseT {
	const char* name;
	void (*func)();
};

struct TestStatsT {
	int total;
	int passed;
	int failed;
};

namespace
{
constexpr int MAX_TEST_CASES = 100;
TestCaseT test_cases[MAX_TEST_CASES];
int test_count = 0;
TestStatsT stats = { 0, 0, 0 };
TestStatsT total_stats = { 0, 0, 0 };
bool current_test_failed = false;
} // namespace
void test_init()
{
	test_count = 0;
	stats = { 0, 0, 0 };
	current_test_failed = false;
	memset(test_cases, 0, sizeof(test_cases));
}

void test_mark_failed() { current_test_failed = true; }

void test_register(const char* name, test_func_t func)
{
	if (test_count >= MAX_TEST_CASES) {
		LOG_ERROR("Maximum number of test cases reached");
		return;
	}

	test_cases[test_count++] = { name, func };
}

void test_run()
{
	stats.total = test_count;

	for (int i = 0; i < test_count; i++) {
		current_test_failed = false;
		test_cases[i].func();

		if (current_test_failed) {
			stats.failed++;
			LOG_TEST("FAIL: %s", test_cases[i].name);
		} else {
			stats.passed++;
		}
	}

	total_stats.total += stats.total;
	total_stats.passed += stats.passed;
	total_stats.failed += stats.failed;

	// The screen console cannot scroll yet, so stay silent unless something
	// failed; the cumulative result is printed by test_print_summary().
	if (stats.failed > 0) {
		LOG_TEST("suite result: total=%d passed=%d failed=%d", stats.total,
				 stats.passed, stats.failed);
	}
}

void run_test_suite(void (*test_suite)(), bool check_leaks)
{
	kernel::log::change_log_level(kernel::log::LogLevel::TEST);
	test_init();
	test_suite();

#ifdef KERNEL_HEAP_DEBUG_ENABLED
	const size_t live_before = kernel::memory::heap_debug::live_bytes();
#endif

	test_run();

#ifdef KERNEL_HEAP_DEBUG_ENABLED
	// Any allocation the suite made but never freed shows up as net growth in
	// the tracked heap. Count it as an extra failed check so the cumulative
	// summary (and CI) flags it, and point at the worst offenders.
	if (check_leaks) {
		const size_t live_after = kernel::memory::heap_debug::live_bytes();
		if (live_after > live_before) {
			total_stats.total++;
			total_stats.failed++;
			LOG_TEST("LEAK: suite leaked %lu bytes",
					 static_cast<unsigned long>(live_after - live_before));
			kernel::memory::heap_debug::report_leaks(5);
		}
	}
#else
	(void)check_leaks;
#endif

	kernel::log::change_log_level(kernel::log::LogLevel::ERROR);
}

bool test_all_passed() { return total_stats.total > 0 && total_stats.failed == 0; }

void test_print_summary()
{
	// printk only emits messages whose level matches the current log level
	// exactly, so the summary must be printed while the level is TEST.
	kernel::log::change_log_level(kernel::log::LogLevel::TEST);

	LOG_TEST("TEST_SUMMARY: total=%d passed=%d failed=%d result=%s",
			 total_stats.total, total_stats.passed, total_stats.failed,
			 test_all_passed() ? "PASS" : "FAIL");

	kernel::log::change_log_level(kernel::log::LogLevel::ERROR);
}
