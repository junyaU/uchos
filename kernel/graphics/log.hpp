/**
 * @file log.hpp
 * @brief Kernel logging system with level-based filtering
 */

#pragma once

#include <libs/common/types.hpp>

namespace kernel::graphics
{

/**
 * @brief Log levels for kernel messages
 *
 * Different log levels allow filtering messages based on severity
 * and purpose. Higher levels indicate more critical messages.
 */
enum class LogLevel : uint8_t {
	DEBUG, ///< Debug messages for development
	ERROR, ///< Error messages indicating problems
	INFO,  ///< Informational messages
	TEST,  ///< Test-specific messages
};

/**
 * @brief Change the minimum log level for output
 *
 * Sets the minimum log level that will be displayed. Messages with
 * a level below this threshold will be suppressed.
 *
 * @param level New minimum log level
 */
void change_log_level(LogLevel level);

} // namespace kernel::graphics

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
__attribute__((no_caller_saved_registers)) void
printk(kernel::graphics::LogLevel level, const char* format, ...);

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
#define LOG_DEBUG(fmt, ...)                                                         \
	LOG(kernel::graphics::LogLevel::DEBUG, fmt, ##__VA_ARGS__)

/**
 * @brief Info log macro
 *
 * Use for general informational messages about system state.
 *
 * @param fmt Format string
 * @param ... Format arguments
 */
#define LOG_INFO(fmt, ...) LOG(kernel::graphics::LogLevel::INFO, fmt, ##__VA_ARGS__)

/**
 * @brief Error log macro
 *
 * Use for error conditions and failure notifications.
 *
 * @param fmt Format string
 * @param ... Format arguments
 */
#define LOG_ERROR(fmt, ...)                                                         \
	LOG(kernel::graphics::LogLevel::ERROR, fmt, ##__VA_ARGS__)

/**
 * @brief Test log macro
 *
 * Use specifically for test output and test result reporting.
 *
 * @param fmt Format string
 * @param ... Format arguments
 */
#define LOG_TEST(fmt, ...) LOG(kernel::graphics::LogLevel::TEST, fmt, ##__VA_ARGS__)
