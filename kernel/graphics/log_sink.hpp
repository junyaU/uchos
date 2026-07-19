/**
 * @file log_sink.hpp
 * @brief On-screen sink for the kernel log
 */

#pragma once

namespace kernel::graphics
{

/**
 * @brief Render one kernel log line on the screen
 *
 * Registered with the log module by graphics initialization; the log module
 * calls it through the sink pointer so it never depends on graphics at
 * compile time.
 *
 * @param line Null-terminated log line to draw
 */
void screen_log_sink(const char* line);

} // namespace kernel::graphics
