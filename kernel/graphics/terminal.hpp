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
	static const int COLUMN_CHARS = 30, ROW_CHARS = 98, LINE_SPACING = 3,
					 START_X = 7, START_Y = 7;
	terminal(Color font_color, const char* user_name, Color user_name_color);

	std::array<std::shared_ptr<file_descriptor>, 3> fds_;

	void initialize_fds();

	size_t print(const char* s);

	size_t print(const char* s, size_t len);

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

	void initialize_command_line();

private:
	static int adjusted_x(int x) { return x * kfont->width() + START_X; }
	static int adjusted_y(int y)
	{
		return y * kfont->height() + START_Y + (y * LINE_SPACING);
	}

	void put_char(char32_t c, int size);

	int user_name_length() const { return strlen(user_name_) + 4; }

	void next_line();
	void scroll_lines();

	void show_user_name();

	std::array<std::array<char32_t, ROW_CHARS + 1>, COLUMN_CHARS> buffer_;
	int cursor_y_{ 0 };
	int cursor_x_{ 0 };
	Color font_color_;
	Color user_name_color_;
	char user_name_[16];
	bool cursor_visible_{ false };
	task_t task_id_{ -1 };

	std::unique_ptr<shell::controller> shell_;
};

extern terminal* main_terminal;
void initialize_terminal();
void task_terminal();

template<typename... Args>
void printk(int level, const char* format, Args... args)
{
	if (main_terminal == nullptr) {
		return;
	}

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

	const int task_id = main_terminal->task_id();
	if (task_id == -1 || task_id == CURRENT_TASK->id) {
		switch (level) {
			case KERN_DEBUG:
				main_terminal->print(s);
				break;
			case KERN_ERROR:
				main_terminal->error(s);
				break;
			case KERN_INFO:
				main_terminal->info(s);
				break;
		}
	} else {
		const message m{ NOTIFY_WRITE, task_id, { .write = { s, level } } };
		send_message(main_terminal->task_id(), &m);
	}
}

struct term_file_descriptor : public file_descriptor {
	term_file_descriptor() = default;
	size_t read(void* buf, size_t len) override;
	size_t write(const void* buf, size_t len) override;
};
