#include "framework.hpp"
#include "graphics/log.hpp"
#include <cstring>

struct test_case_t {
	const char* name;
	void (*func)();
};

struct test_stats_t {
	int total;
	int passed;
	int failed;
};

namespace
{
constexpr int MAX_TEST_CASES = 100;
test_case_t test_cases[MAX_TEST_CASES];
int test_count = 0;
test_stats_t stats = { 0, 0, 0 };
bool current_test_failed = false;
} // namespace
void test_init()
{
	test_count = 0;
	stats = { 0, 0, 0 };
	current_test_failed = false;
	memset(test_cases, 0, sizeof(test_cases));
}

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
		LOG_INFO("Running test: %s", test_cases[i].name);
		test_cases[i].func();

		if (!current_test_failed) {
			stats.passed++;
		}
	}
}

void run_test_suite(void (*test_suite)())
{
	test_init();
	test_suite();
	test_run();
}
