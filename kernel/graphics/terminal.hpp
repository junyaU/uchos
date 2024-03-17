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
#include "../graphics/color.hpp"
#include "../graphics/font.hpp"
#include "../task/ipc.hpp"
#include "../task/task.hpp"
#include "../types.hpp"
#include "screen.hpp"
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <memory>

namespace shell
{
class controller;
} // namespace shell

class terminal
{
public:
	static constexpr int COLUMN_CHARS = 30, ROW_CHARS = 98, LINE_SPACING = 3,
						 START_X = 7, START_Y = 7;
	terminal(Color font_color, const char* user_name, Color user_name_color);

	std::array<std::shared_ptr<file_descriptor>, 3> fds_;

	void initialize_fds();

	size_t print(const char* s, Color color = WHITE);

	size_t
	print(const char* s, size_t len, Color color = WHITE, bool is_input = false);

	void error(const char* s);

	void info(const char* s);

	template<typename... Args>
	void printf(const char* format, Args... args)
	{
		char s[1024];
		sprintf(s, format, args...);
		print(s);
	}

	task_t task_id() const { return task_id_; }

	void set_task_id(task_t id) { task_id_ = id; }

	void print_interrupt_hex(uint64_t value);

	void input_key(uint8_t c);

	void cursor_blink();

	void clear();

	void debug_mode() { is_debug_ = true; }

private:
	static int adjusted_x(int x) { return x * kfont->width() + START_X; }
	static int adjusted_y(int y)
	{
		return y * kfont->height() + START_Y + (y * LINE_SPACING);
	}

	void put_char(char32_t c, int size, Color color, bool is_input);

	int user_name_length() const { return strlen(user_name_) + 4; }

	void next_line(bool is_input = false);
	void scroll_lines();

	void clear_input_line();

	void show_user_name();

	bool is_debug_{ false };

	std::array<std::array<char32_t, ROW_CHARS + 1>, COLUMN_CHARS> buffer_;
	std::array<std::array<uint32_t, ROW_CHARS>, COLUMN_CHARS> color_buffer_;
	int cursor_y_{ 0 };
	int cursor_x_{ 0 };
	Color font_color_;
	Color user_name_color_;
	char user_name_[16];
	bool cursor_visible_{ false };
	task_t task_id_{ -1 };
};

extern terminal* main_terminal;
void initialize_terminal();
void task_terminal();

static int kernel_cursor_x = 0;
static int kernel_cursor_y = 5;
static int LOG_LEVEL = KERN_ERROR;

template<typename... Args>
__attribute__((no_caller_saved_registers)) void
printk(int level, const char* format, Args... args)
{
	char s[1024];
	if (sizeof...(args) == 0) {
		strncpy(s, format, sizeof(s));
	} else {
		// TODO: fix this warning
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-security"
		sprintf(s, format, args...);
#pragma GCC diagnostic pop
	}

	if (level != LOG_LEVEL) {
		return;
	}

	for (size_t i = 0; i < strlen(s) && s[i] != '\0'; ++i) {
		write_ascii(*kscreen, { kernel_cursor_x++ * 8, kernel_cursor_y * 16 }, s[i],
					0xffff00);

		if (s[i] == '\n') {
			kernel_cursor_x = 0;
			++kernel_cursor_y;
			continue;
		}

		if (kernel_cursor_x >= 98) {
			kernel_cursor_x = 0;
			++kernel_cursor_y;
		}
	}

	kernel_cursor_x = 0;
	++kernel_cursor_y;
}

struct term_file_descriptor : public file_descriptor {
	term_file_descriptor() = default;
	size_t read(void* buf, size_t len) override;
	size_t write(const void* buf, size_t len) override;
};
