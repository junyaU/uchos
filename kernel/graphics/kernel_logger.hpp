#pragma once

#include <array>
#include <cstdint>

#include "color.hpp"

class kernel_logger
{
public:
	static const int COLUMN_CHARS = 25, ROW_CHARS = 80;
	kernel_logger(Color font_color);

	void print(const char* s);
	void printf(const char* format, ...);

	void clear();

private:
	void next_line();
	void scroll_lines();

	char buffer_[COLUMN_CHARS][ROW_CHARS + 1];
	int cursor_y_{ 0 };
	int cursor_x_{ 0 };
	Color font_color_;
};

extern kernel_logger* klogger;

void initialize_kernel_logger();