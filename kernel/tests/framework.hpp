/**
 * @file tests/framework.hpp
 * @brief Kernel unit testing framework
 *
 * This file provides a simple unit testing framework for the kernel.
 * It allows registration and execution of test functions, helping to
 * ensure kernel components work correctly through automated testing.
 *
 * @date 2024
 */

#pragma once

/**
 * @brief Type definition for test functions
 *
 * Test functions take no parameters and return void. They should use
 * assertion macros to verify test conditions.
 */
using test_func_t = void (*)();

/**
 * @brief Initialize the test framework
 *
 * Prepares the test framework for use. This must be called before
 * registering or running any tests. It clears any previously registered
 * tests and resets test statistics.
 */
void test_init();

/**
 * @brief Register a test function
 *
 * Adds a test function to the test registry. The test will be executed
 * when test_run() is called.
 *
 * @param name Descriptive name for the test
 * @param func Test function to register
 *
 * @note Test names should be unique and descriptive
 */
void test_register(const char* name, test_func_t func);

/**
 * @brief Run all registered tests
 *
 * Executes all tests that have been registered with test_register(). Each
 * failing test is logged individually as it runs (`FAIL: <name>`); the
 * screen console cannot scroll yet, so nothing is printed for a suite that
 * passes entirely except the per-suite `total=/passed=/failed=` line, which
 * is only emitted when at least one test failed. The cumulative result
 * across all suites is reported separately by test_print_summary().
 *
 * @note Tests are run in the order they were registered
 */
void test_run();

/**
 * @brief Mark the currently running test as failed
 *
 * Called by the assertion macros in tests/macros.hpp when a check fails.
 * The failure is counted in the statistics reported by test_run().
 */
void test_mark_failed();

/**
 * @brief Print the cumulative test results across all executed suites
 *
 * Prints a machine-readable summary line
 * (TEST_SUMMARY: total=N passed=N failed=N result=PASS|FAIL) covering every
 * suite executed since boot. Call once after all test suites have run so
 * CI can parse the overall result.
 *
 * @note result is PASS only if at least one test ran and none failed
 */
void test_print_summary();

/**
 * @brief Check the cumulative test result across all executed suites
 *
 * Reflects the same totals reported by test_print_summary(): the result
 * across every suite run since boot.
 *
 * @return true if at least one test ran and none of them failed
 */
bool test_all_passed();

/**
 * @brief Run a complete test suite
 *
 * Convenience function that initializes the test framework, runs the
 * provided test suite function (which should register tests), and then
 * executes all registered tests.
 *
 * When the kernel is built with KERNEL_HEAP_DEBUG, the used heap is snapshotted
 * around the suite and any net growth is reported as an extra failure. Suites
 * that intentionally retain allocations past their own lifetime (e.g. ones that
 * create tasks or populate a global cache) must opt out with check_leaks=false.
 *
 * @param test_suite Function that registers all tests in the suite
 * @param check_leaks Fail the suite on a net heap increase (heap-debug builds
 * only; ignored otherwise). Defaults to true.
 *
 * @example
 * void register_memory_tests() {
 *     test_register("test_allocation", test_allocation);
 *     test_register("test_free", test_free);
 * }
 * run_test_suite(register_memory_tests);
 */
void run_test_suite(void (*test_suite)(), bool check_leaks = true);
