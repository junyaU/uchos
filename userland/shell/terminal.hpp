#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

struct shell;

constexpr size_t TERMINAL_WIDTH = 98;
constexpr size_t TERMINAL_HEIGHT = 35;

struct terminal {
	static std::array<std::array<char, TERMINAL_WIDTH>, TERMINAL_HEIGHT> buffer;
	static std::array<std::array<uint32_t, TERMINAL_WIDTH>, TERMINAL_HEIGHT>
			color_buffer;
	char user_name[16];
	int cursor_x;
	int cursor_y;
	char input[TERMINAL_WIDTH];
	int input_index;
	int prompt_len;
	shell* shell_;

	terminal(shell* s);

	void scroll();

	void new_line();

	void print(char s, uint32_t color = 0xffffff);

	void print(const char* s, uint32_t color = 0xffffff);

	size_t print_user();

	void input_char(char c);
};
