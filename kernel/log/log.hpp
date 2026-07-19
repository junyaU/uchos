/**
 * @file log.hpp
 * @brief Kernel logging system
 *
 * The serial console always receives every message at every level: it is the
 * only output visible in headless runs and serves as the full boot trace.
 * The screen shows only messages whose level exactly matches the selected
 * level, and only after a screen sink has been registered (the graphics
 * module registers one when it initializes).
 */

#pragma once

#include <libs/common/types.hpp>

namespace kernel::log
{

/**
 * @brief Log levels for kernel messages
 *
 * Levels are independent categories, not severities: selecting a level
 * chooses which category appears on screen. The serial log is unaffected
 * and always records every level.
 */
enum class LogLevel : uint8_t {
	DEBUG, ///< Debug messages for development
	ERROR, ///< Error messages indicating problems
	INFO,  ///< Informational messages
	TEST,  ///< Test-specific messages
};

/**
 * @brief Select the log level shown on screen
 *
 * Only messages whose level exactly matches the selected level are drawn on
 * screen. Serial output is unaffected.
 *
 * @param level Level to show on screen
 */
void change_log_level(LogLevel level);

/// Screen sink: receives one null-terminated log line to display
using ScreenSink = void (*)(const char* line);

/**
 * @brief Register the on-screen output callback
 *
 * Called by the graphics module once the screen is ready. Until a sink is
 * registered, log output goes to the serial console only, which keeps
 * logging safe during early boot. The log module itself has no compile-time
 * dependency on graphics.
 *
 * @param sink Callback that renders one log line, or nullptr to detach
 */
void register_screen_sink(ScreenSink sink);

} // namespace kernel::log

/**
 * @brief Kernel print function with log level support
 *
 * Printf-style function for kernel logging. Supports format specifiers
 * and variable arguments. The no_caller_saved_registers attribute
 * ensures this function can be called from interrupt contexts safely.
 *
 * @param level Log level for this message
 * @param format Printf-style format string
 * @param ... Variable arguments matching format specifiers
 *
 * @note This function preserves all caller-saved registers
 */
__attribute__((no_caller_saved_registers)) void printk(kernel::log::LogLevel level,
													   const char* format,
													   ...);

/**
 * @brief Base logging macro with file location information
 *
 * Automatically prepends function name and line number to log messages.
 *
 * @param level Log level
 * @param fmt Format string
 * @param ... Format arguments
 */
#define LOG(level, fmt, ...)                                                        \
	printk(level, "[%s:%d] " fmt "", __FILE__, __LINE__, ##__VA_ARGS__)

/**
 * @brief Debug log macro
 *
 * Use for detailed debugging information during development.
 *
 * @param fmt Format string
 * @param ... Format arguments
 */
#define LOG_DEBUG(fmt, ...) LOG(kernel::log::LogLevel::DEBUG, fmt, ##__VA_ARGS__)

/**
 * @brief Info log macro
 *
 * Use for general informational messages about system state.
 *
 * @param fmt Format string
 * @param ... Format arguments
 */
#define LOG_INFO(fmt, ...) LOG(kernel::log::LogLevel::INFO, fmt, ##__VA_ARGS__)

/**
 * @brief Error log macro
 *
 * Use for error conditions and failure notifications.
 *
 * @param fmt Format string
 * @param ... Format arguments
 */
#define LOG_ERROR(fmt, ...) LOG(kernel::log::LogLevel::ERROR, fmt, ##__VA_ARGS__)

/**
 * @brief Test log macro
 *
 * Use specifically for test output and test result reporting.
 *
 * @param fmt Format string
 * @param ... Format arguments
 */
#define LOG_TEST(fmt, ...) LOG(kernel::log::LogLevel::TEST, fmt, ##__VA_ARGS__)
