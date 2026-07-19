#pragma once

/**
 * @brief Register the heap-debug instrumentation tests
 *
 * The suite verifies that each of the four heap faults (leak, double free,
 * use-after-free, buffer overflow) is caught. It registers no tests unless the
 * kernel is built with KERNEL_HEAP_DEBUG, since the instrumentation is compiled
 * out otherwise.
 */
void register_heap_debug_tests();
