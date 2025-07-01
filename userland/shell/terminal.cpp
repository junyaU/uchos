#include "terminal.hpp"
#include "libs/common/types.hpp"
#include <libs/common/process_id.hpp>
#include "shell.hpp"
#include <cstdarg>
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <libs/user/ipc.hpp>
#include <libs/user/print.hpp>
#include <libs/user/time.hpp>

std::array<std::array<char, TERMINAL_WIDTH>, TERMINAL_HEIGHT> terminal::buffer;
std::array<std::array<uint32_t, TERMINAL_WIDTH>, TERMINAL_HEIGHT>
		terminal::color_buffer;

terminal::terminal(shell* s) : shell_(s)
{
	char user_name[16] = "root@uchos";
	memcpy(this->user_name, user_name, 16);

	cursor_x = 0;
	cursor_y = 0;
	enable_input = true;
}

void terminal::blink_cursor()
{
	if (!enable_input) {
		cursor_visible = false;
		return;
	}

	if (cursor_visible) {
		print_text(adjusted_x(cursor_x), adjusted_y(cursor_y), "_", 0xffffff);
	} else {
		delete_char(adjusted_x(cursor_x), adjusted_y(cursor_y));
	}

	cursor_visible = !cursor_visible;
}

int terminal::adjusted_x(int x) { return x * FONT_WIDTH + START_X; }

int terminal::adjusted_y(int y)
{
	return y * FONT_HEIGHT + START_Y + (y * LINE_SPACING);
}

size_t terminal::print_user()
{
	if (!enable_input) {
		return 0;
	}

	print(user_name, 0x00ff00);
	size_t user_len = strlen(user_name);
	memcpy(input, user_name, user_len);
	input_index = user_len;

	print(":~$ ", 0xffffff);
	memcpy(&input[input_index], ":~$ ", 4);
	input_index += 4;

	prompt_len = user_len + 4;

	return prompt_len;
}

void terminal::scroll()
{
	clear_screen();
	memcpy(&buffer, &buffer[1], sizeof(buffer) - sizeof(buffer[0]));
	memcpy(&color_buffer, &color_buffer[1],
		   sizeof(color_buffer) - sizeof(color_buffer[0]));
	memset(&buffer[TERMINAL_HEIGHT - 1], '\0', TERMINAL_WIDTH);
	memset(&color_buffer[TERMINAL_HEIGHT - 1], 0, TERMINAL_WIDTH);

	for (int y = 0; y < TERMINAL_HEIGHT - 1; ++y) {
		for (int x = 0; x < TERMINAL_WIDTH; ++x) {
			print_text(adjusted_x(x), adjusted_y(y), &buffer[y][x],
					   color_buffer[y][x]);
		}
	}

	cursor_y = TERMINAL_HEIGHT - 1;
	cursor_x = 0;
}

void terminal::new_line()
{
	if (cursor_y == TERMINAL_HEIGHT - 1) {
		scroll();
	} else {
		cursor_x = 0;
		++cursor_y;
	}
}

void terminal::print(char s, uint32_t color)
{
	// delete the cursor
	delete_char(adjusted_x(cursor_x), adjusted_y(cursor_y));

	if (s == '\n') {
		new_line();
		return;
	}

	char str[2] = { s, '\0' };
	print_text(adjusted_x(cursor_x), adjusted_y(cursor_y), str, color);
	buffer[cursor_y][cursor_x] = s;
	color_buffer[cursor_y][cursor_x++] = color;

	if (cursor_x == TERMINAL_WIDTH) {
		cursor_x = 0;
		++cursor_y;
	}
}

void terminal::print(const char* s, uint32_t color)
{
	// TODO: bulk print
	size_t len = strlen(s);
	for (size_t i = 0; i < len; i++) {
		print(s[i], color);
	}
}

void terminal::printf(const char* format, ...)
{
	va_list args;
	va_start(args, format);
	char buffer[512];
	vsprintf(buffer, format, args);
	va_end(args);

	print(buffer);
}

void terminal::print_message(char* s, bool is_end_of_message)
{
	print(s);
	enable_input = is_end_of_message;
	if (is_end_of_message) {
		print("\n");
		print_user();
	}
}

void terminal::input_char(char c)
{
	if (!enable_input) {
		return;
	}

	// delete the cursor
	delete_char(adjusted_x(cursor_x), adjusted_y(cursor_y));
	input[input_index] = '\0';

	if (c == '\n') {
		input_index = 0;

		new_line();
		shell_->process_input(&input[prompt_len], *this);

		memset(input, '\0', TERMINAL_WIDTH);
		print_user();

		return;
	}

	if (c == '\b') {
		if (input_index > prompt_len) {
			input[--input_index] = '\0';
			delete_char(adjusted_x(--cursor_x), adjusted_y(cursor_y));
			buffer[cursor_y][cursor_x] = '\0';
		}

		return;
	}

	input[input_index++] = c;
	print(c);
}

void set_cursor_timer(int ms)
{
	set_timer(ms, true, timeout_action_t::TERMINAL_CURSOR_BLINK, process_ids::SHELL.raw());
}
