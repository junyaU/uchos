#pragma once

#include <array>
#include <cstddef>

struct terminal {
	std::array<std::array<char, 98>, 30> buffer;
	char user_name[16];
	int cursor_x;
	int cursor_y;
	char input[98];
	int input_index;
	int prompt_len;

	terminal();

	void new_line();

	void print(char s);

	void print(const char* s);

	size_t print_user();

	void input_char(char c);
};
