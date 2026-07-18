/**
 * @file tests/runner.hpp
 * @brief Staged kernel test runner
 *
 * Groups all test suites into three stages that main.cpp invokes at the
 * points in the boot sequence where their preconditions hold. When the
 * KERNEL_TESTS CMake option is OFF the stages compile to empty inline
 * stubs and no test code is linked into the kernel.
 */

#pragma once

namespace kernel::tests
{

#ifdef KERNEL_TESTS_ENABLED
/**
 * @brief Run suites that need the bootstrap allocator to be alive
 *
 * Must be called after kernel::memory::initialize() and before
 * kernel::memory::release_bootstrap(), because the bootstrap allocator
 * suite exercises the global boot_allocator directly.
 */
void run_bootstrap_stage_tests();

/**
 * @brief Run suites that need the timer to be idle
 *
 * Must be called after kernel::timers::initialize() and before
 * kernel::timers::local_apic::initialize(), because the timer suite
 * assumes tick 0 and manually advances the tick counter.
 */
void run_timer_stage_tests();

/**
 * @brief Run all remaining suites and print the cumulative summary
 *
 * Must be called after kernel::task::initialize(). Task slots created
 * by test cases are released afterwards so the task table is left
 * usable for the rest of the boot.
 */
void run_main_stage_tests();
#else
inline void run_bootstrap_stage_tests() {}
inline void run_timer_stage_tests() {}
inline void run_main_stage_tests() {}
#endif

} // namespace kernel::tests
