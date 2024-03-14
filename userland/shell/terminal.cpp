#include "terminal.hpp"
#include <../../libs/user/print.hpp>
#include <cstddef>
#include <cstring>

std::array<std::array<char, 98>, 35> terminal::buffer;
std::array<std::array<uint32_t, 98>, 35> terminal::color_buffer;

terminal::terminal()
{
	char user_name[16] = "root@uchos";
	memcpy(this->user_name, user_name, 16);

	cursor_x = 0;
	cursor_y = 0;
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

	memset(&buffer[34], '\0', 98);

	for (int i = 0; i < 34; ++i) {
		for (int j = 0; j < 98; ++j) {
			print_text(j, i, &buffer[i][j], color_buffer[i][j]);
		}
	}

	cursor_y = 34;
	cursor_x = 0;
}

void terminal::new_line()
{
	if (cursor_y == 34) {
		scroll();
	} else {
		cursor_x = 0;
		cursor_y += 1;
	}
}

void terminal::print(char s, uint32_t color)
{
	if (s == '\n') {
		new_line();
		return;
	}

	char str[2] = { s, '\0' };
	print_text(cursor_x, cursor_y, str, color);
	buffer[cursor_y][cursor_x] = s;
	color_buffer[cursor_y][cursor_x++] = color;

	if (cursor_x == 98) {
		cursor_x = 0;
		cursor_y += 1;
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

		// TODO: handle input to shell
		print(&input[prompt_len]);
		print(" : command not found");
		print("\n");

		memset(input, '\0', 98);
		print_user();

		return;
	}

	if (c == '\b') {
		if (input_index > prompt_len) {
			input[--input_index] = '\0';
			delete_char(--cursor_x, cursor_y);
			buffer[cursor_y][cursor_x] = '\0';
		}

		return;
	}

	input[input_index++] = c;
	print(c);
}
