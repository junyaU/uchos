#include "kernel_logger.hpp"

#include <cstdio>
#include <stdarg.h>

#include "font.hpp"
#include "screen.hpp"

kernel_logger::kernel_logger(Color font_color) : font_color_{ font_color }
{
	clear();
}

void kernel_logger::print(const char* s)
{
	while (*s) {
		if (*s == '\n') {
			next_line();
			s++;
			continue;
		}

		buffer_[cursor_y_][cursor_x_] = *s;
		screen->DrawString(Point2D{ cursor_x_ * bitmap_font->Width(),
									cursor_y_ * bitmap_font->Height() },
						   *s, font_color_.GetCode());

		if (cursor_x_ == ROW_CHARS - 1) {
			next_line();
		} else if (cursor_x_ < ROW_CHARS - 1) {
			cursor_x_++;
		}

		s++;
	}
}

void kernel_logger::printf(const char* format, ...)
{
	char s[1024];

	va_list ap;
	va_start(ap, format);
	vsprintf(s, format, ap);
	va_end(ap);

	print(s);
}

void kernel_logger::clear()
{
	for (int y = 0; y < kernel_logger::COLUMN_CHARS; y++) {
		memset(buffer_[y], '\0', sizeof(buffer_[y]));
	}

	cursor_x_ = 0;
	cursor_y_ = 0;

	screen->FillRectangle(
			Point2D{ 0, 0 },
			Point2D{ kernel_logger::ROW_CHARS * bitmap_font->Width(),
					 kernel_logger::COLUMN_CHARS * bitmap_font->Height() },
			screen->BgColor().GetCode());
}

void kernel_logger::next_line()
{
	if (cursor_y_ == COLUMN_CHARS - 1) {
		scroll_lines();
	} else {
		cursor_x_ = 0;
		cursor_y_++;
	}
}

void kernel_logger::scroll_lines()
{
	screen->FillRectangle(
			Point2D{ 0, 0 },
			Point2D{ kernel_logger::ROW_CHARS * bitmap_font->Width(),
					 kernel_logger::COLUMN_CHARS * bitmap_font->Height() },
			screen->BgColor().GetCode());

	memcpy(buffer_[0], buffer_[1], sizeof(buffer_) - sizeof(buffer_[0]));

	for (int x = 0; x < kernel_logger::ROW_CHARS; x++) {
		buffer_[kernel_logger::COLUMN_CHARS - 1][x] = '\0';
	}

	for (int y = 0; y < kernel_logger::COLUMN_CHARS - 1; y++) {
		for (int x = 0; x < kernel_logger::ROW_CHARS; x++) {
			screen->DrawString(
					Point2D{ x * bitmap_font->Width(), y * bitmap_font->Height() },
					buffer_[y][x], font_color_.GetCode());
		}
	}

	cursor_x_ = 0;
	cursor_y_ = kernel_logger::COLUMN_CHARS - 1;
}

kernel_logger* klogger;
char kernel_logger_buf[sizeof(kernel_logger)];

void initialize_kernel_logger()
{
	klogger = new (kernel_logger_buf) kernel_logger{ Color{ 255, 255, 255 } };
}
