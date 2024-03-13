#pragma once

#include <array>
#include <cstdint>

struct terminal {
	std::array<std::array<char, 98>, 30> buffer;
	char user_name[16];
	int cursor_x;
	int cursor_y;

	terminal();

	void print(const char* s);

	void print_user();
};
