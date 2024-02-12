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

#include "../types.hpp"
#include "color.hpp"
#include "font.hpp"
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <memory>

namespace command_line
{
class controller;
} // namespace command_line

class terminal
{
public:
	static const int COLUMN_CHARS = 30, ROW_CHARS = 98, LINE_SPACING = 3,
					 START_X = 7, START_Y = 7;
	terminal(Color font_color, const char* user_name, Color user_name_color);

	void print(const char* s);

	void error(const char* s);

	void info(const char* s);

	template<typename... Args>
	void printf(const char* format, Args... args)
	{
		char s[1024];
		sprintf(s, format, args...);
		print(s);
	}

	template<typename... Args>
	void errorf(const char* format, Args... args)
	{
		char s[1024];
		sprintf(s, format, args...);
		error(s);
	}

	task_t task_id() const { return task_id_; }

	void set_task_id(task_t id) { task_id_ = id; }

	void print_interrupt_hex(uint64_t value);

	void input_key(uint8_t c);

	void cursor_blink();

	void clear();

	void initialize_command_line();

private:
	static int adjusted_x(int x) { return x * kfont->width() + START_X; }
	static int adjusted_y(int y)
	{
		return y * kfont->height() + START_Y + (y * LINE_SPACING);
	}

	int user_name_length() const { return strlen(user_name_) + 4; }

	void next_line();
	void scroll_lines();

	void show_user_name();

	char buffer_[COLUMN_CHARS][ROW_CHARS + 1];
	int cursor_y_{ 0 };
	int cursor_x_{ 0 };
	Color font_color_;
	Color user_name_color_;
	char user_name_[16];
	bool cursor_visible_{ false };
	task_t task_id_{ -1 };

	std::unique_ptr<command_line::controller> cl_ctrl_;
};

extern terminal* main_terminal;
void initialize_terminal();
void task_terminal();

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wvariadic-macros"
void printk(int level, const char* format, ...);
#pragma GCC diagnostic pop