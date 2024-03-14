#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

struct terminal {
	static std::array<std::array<char, 98>, 35> buffer;
	static std::array<std::array<uint32_t, 98>, 35> color_buffer;
	char user_name[16];
	int cursor_x;
	int cursor_y;
	char input[98];
	int input_index;
	int prompt_len;

	terminal();

	void scroll();

	void new_line();

	void print(char s, uint32_t color = 0xffffff);

	void print(const char* s, uint32_t color = 0xffffff);

	size_t print_user();

	void input_char(char c);
};
