#pragma once

/**
 * @brief Register stdio test cases
 * 
 * This function registers all stdio-related test cases with the test framework.
 * Tests include:
 * - FD table initialization
 * - Writing to stdout/stderr
 * - Reading from stdin
 * - Invalid FD handling
 * - FD inheritance between processes
 */
void register_stdio_tests();