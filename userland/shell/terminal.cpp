#include "terminal.hpp"
#include <../../libs/user/print.hpp>
#include <cstddef>
#include <cstring>

terminal::terminal()
{
	char user_name[16] = "root@uchos";
	memcpy(this->user_name, user_name, 16);

	cursor_x = 0;
	cursor_y = 0;
	for (int i = 0; i < 30; i++) {
		for (int j = 0; j < 98; j++) {
			buffer[i][j] = '\0';
		}
	}
}

size_t terminal::print_user()
{
	size_t total = 0;
	print_text(cursor_x, cursor_y, user_name, 0x00ff00);
	size_t user_name_len = strlen(user_name);
	cursor_x += user_name_len;
	total += user_name_len;

	for (size_t i = 0; i < user_name_len; i++) {
		buffer[cursor_y][cursor_x + i] = user_name[i];
	}

	print_text(cursor_x, cursor_y, ":~$ ", 0xffffff);
	cursor_x += 4;
	total += 4;

	for (size_t i = 0; i < prompt_len; i++) {
		buffer[cursor_y][cursor_x + i] = ":~$ "[i];
	}

	memcpy(&input[input_index], user_name, user_name_len);
	input_index += user_name_len;
	memcpy(&input[input_index], ":~$ ", 4);
	input_index += 4;

	prompt_len = total;

	return total;
}

void terminal::new_line()
{
	cursor_x = 0;
	cursor_y += 1;
}

void terminal::print(char s)
{
	if (s == '\n') {
		new_line();
		return;
	}

	char str[2] = { s, '\0' };
	print_text(cursor_x, cursor_y, str, 0xffffff);
	buffer[cursor_y][cursor_x++] = s;

	if (cursor_x == 98) {
		cursor_x = 0;
		cursor_y += 1;
	}

	if (cursor_y == 30) {
		cursor_y = 0;
	}
}

void terminal::print(const char* s)
{
	size_t len = strlen(s);
	for (size_t i = 0; i < len; i++) {
		print(s[i]);
	}
}

void terminal::input_char(char c)
{
	if (c == '\n') {
		input[input_index] = '\0';
		input_index = 0;

		cursor_x = 0;
		++cursor_y;

		// TODO: handle input to shell
		print(&input[prompt_len]);
		print(" : command not found");
		print("\n");

		// print(input);
		memset(input, 0, 98);

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
