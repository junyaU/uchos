#include "terminal.hpp"
#include "shell.hpp"
#include <../../libs/user/print.hpp>
#include <cstddef>
#include <cstring>

std::array<std::array<char, TERMINAL_WIDTH>, TERMINAL_HEIGHT> terminal::buffer;
std::array<std::array<uint32_t, TERMINAL_WIDTH>, TERMINAL_HEIGHT>
		terminal::color_buffer;

terminal::terminal(shell* s) : shell_(s)
{
	char user_name[16] = "root@uchos";
	memcpy(this->user_name, user_name, 16);

	cursor_x = 0;
	cursor_y = 0;
}

int terminal::adjusted_x(int x) { return x * FONT_WIDTH + START_X; }

int terminal::adjusted_y(int y)
{
	return y * FONT_HEIGHT + START_Y + (y * LINE_SPACING);
}

size_t terminal::print_user()
{
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

void terminal::input_char(char c)
{
	if (c == '\n') {
		input[input_index] = '\0';
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
