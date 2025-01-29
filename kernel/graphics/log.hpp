/*
 * @file graphics/terminal.hpp
 *
 * @brief kernel terminal
 *
 * Terminal class for kernel-level interaction. Provides functionality for
 * displaying text and handling user input in a graphical interface. Supports
 * operations such as printing strings, formatted text, handling user commands,
 * and displaying system messages. Also includes functionality to clear the
 * screen, scroll through the terminal history, and manage user input.
 *
 */

#pragma once

#include <libs/common/types.hpp>

enum class log_level : uint8_t {
	DEBUG,
	ERROR,
	INFO,
	TEST,
};

void change_log_level(log_level level);

__attribute__((no_caller_saved_registers)) void
printk(log_level level, const char* format, ...);

#define LOG(level, fmt, ...)                                                        \
	printk(level, "[%s:%d] " fmt "", __func__, __LINE__, ##__VA_ARGS__)

#define LOG_DEBUG(fmt, ...) LOG(log_level::DEBUG, fmt, ##__VA_ARGS__)
#define LOG_INFO(fmt, ...) LOG(log_level::INFO, fmt, ##__VA_ARGS__)
#define LOG_ERROR(fmt, ...) LOG(log_level::ERROR, fmt, ##__VA_ARGS__)
#define LOG_TEST(fmt, ...) LOG(log_level::TEST, fmt, ##__VA_ARGS__)
