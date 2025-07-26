#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

struct Shell;

constexpr size_t TERMINAL_WIDTH = 98, TERMINAL_HEIGHT = 30;
constexpr int LINE_SPACING = 3, START_X = 7, START_Y = 7;

struct Terminal {
	static std::array<std::array<char, TERMINAL_WIDTH>, TERMINAL_HEIGHT> buffer;
	static std::array<std::array<uint32_t, TERMINAL_WIDTH>, TERMINAL_HEIGHT> color_buffer;
	char user_name[16];
	char current_dir[12];
	int cursor_x;
	int cursor_y;
	char input[TERMINAL_WIDTH];
	int input_index;
	int prompt_len;
	bool cursor_visible;
	bool enable_input;
	Shell* shell_;

	Terminal(Shell* s);

	void blink_cursor();

	static int adjusted_x(int x);

	static int adjusted_y(int y);

	void scroll();

	void new_line();

	void print(char s, uint32_t color = 0xffffff);

	void print(const char* s, uint32_t color = 0xffffff);

	void printf(const char* format, ...);

	size_t print_user();

	void input_char(char c);

	void register_current_dir(const char* name);
};

void set_cursor_timer(int ms);
