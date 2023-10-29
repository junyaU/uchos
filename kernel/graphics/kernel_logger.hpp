#pragma once

#include "color.hpp"
#include "font.hpp"
#include <stdio.h>

class kernel_logger
{
public:
	static const int COLUMN_CHARS = 30, ROW_CHARS = 98, LINE_SPACING = 3,
					 START_X = 7, START_Y = 7;

	kernel_logger(Color font_color);

	void print(const char* s);

	template<typename... Args>
	void printf(const char* format, Args... args)
	{
		char s[1024];
		sprintf(s, format, args...);
		print(s);
	}

	void clear();

private:
	static int adjusted_x(int x) { return x * bitmap_font->Width() + START_X; }
	static int adjusted_y(int y)
	{
		return y * bitmap_font->Height() + START_Y + (y * LINE_SPACING);
	}

	void next_line();
	void scroll_lines();

	char buffer_[COLUMN_CHARS][ROW_CHARS + 1];
	int cursor_y_{ 0 };
	int cursor_x_{ 0 };
	Color font_color_;
};

extern kernel_logger* klogger;

void initialize_kernel_logger();