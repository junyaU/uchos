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

#include "../file_system/file_descriptor.hpp"
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <libs/common/types.hpp>

static int kernel_cursor_x = 0;
static int kernel_cursor_y = 5;
static int CURRENT_LOG_LEVEL = KERN_ERROR;

__attribute__((no_caller_saved_registers)) void
printk(int level, const char* format, ...);

struct term_file_descriptor : public file_descriptor {
	term_file_descriptor() = default;
	size_t read(void* buf, size_t len) override;
	size_t write(const void* buf, size_t len) override;
};

#define LOG(level, fmt, ...)                                                        \
	printk(level, "[%s:%d] " fmt "", __func__, __LINE__, ##__VA_ARGS__)

#define LOG_DEBUG(fmt, ...) LOG(KERN_DEBUG, fmt, ##__VA_ARGS__)
#define LOG_INFO(fmt, ...) LOG(KERN_INFO, fmt, ##__VA_ARGS__)
#define LOG_ERROR(fmt, ...) LOG(KERN_ERROR, fmt, ##__VA_ARGS__)
