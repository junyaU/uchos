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
 * Executes all tests that have been registered with test_register().
 * Prints results to the kernel log, including pass/fail status and
 * any error messages.
 * 
 * @note Tests are run in the order they were registered
 */
void test_run();

/**
 * @brief Run a complete test suite
 * 
 * Convenience function that initializes the test framework, runs the
 * provided test suite function (which should register tests), and then
 * executes all registered tests.
 * 
 * @param test_suite Function that registers all tests in the suite
 * 
 * @example
 * void register_memory_tests() {
 *     test_register("test_allocation", test_allocation);
 *     test_register("test_free", test_free);
 * }
 * run_test_suite(register_memory_tests);
 */
void run_test_suite(void (*test_suite)());
