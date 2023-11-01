#include "kernel_logger.hpp"
#include "color.hpp"
#include "font.hpp"
#include "screen.hpp"
#include <new>
#include <string.h>

kernel_logger::kernel_logger(Color font_color) : font_color_{ font_color }
{
	clear();
}

void kernel_logger::print(const char* s)
{
	while (*s != '\0') {
		if (*s == '\n') {
			next_line();
			s++;
			continue;
		}

		buffer_[cursor_y_][cursor_x_] = *s;
		kscreen->draw_string(Point2D{ adjusted_x(cursor_x_), adjusted_y(cursor_y_) },
							 *s, font_color_.GetCode());

		if (cursor_x_ == ROW_CHARS - 1) {
			next_line();
		} else if (cursor_x_ < ROW_CHARS - 1) {
			cursor_x_++;
		}

		s++;
	}
}

void kernel_logger::info(const char* s)
{
	print("[INFO] ");
	print(s);
	print("\n");
}

void kernel_logger::error(const char* s)
{
	print("[ERROR] ");
	print(s);
	print("\n");
}

void kernel_logger::clear()
{
	for (int y = 0; y < kernel_logger::COLUMN_CHARS; y++) {
		memset(buffer_[y], '\0', sizeof(buffer_[y]));
	}

	cursor_x_ = 0;
	cursor_y_ = 0;

	kscreen->fill_rectangle(
			Point2D{ 0, 0 },
			Point2D{ kernel_logger::ROW_CHARS * kfont->width() + START_X,
					 kernel_logger::COLUMN_CHARS * kfont->height() +
							 (kernel_logger::COLUMN_CHARS * LINE_SPACING) +
							 START_Y },
			kscreen->bg_color().GetCode());
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
	kscreen->fill_rectangle(
			Point2D{ 0, 0 },
			Point2D{ adjusted_x(ROW_CHARS), adjusted_y(COLUMN_CHARS) },
			kscreen->bg_color().GetCode());

	memcpy(buffer_[0], buffer_[1], sizeof(buffer_) - sizeof(buffer_[0]));

	for (int x = 0; x < kernel_logger::ROW_CHARS; x++) {
		buffer_[kernel_logger::COLUMN_CHARS - 1][x] = '\0';
	}

	for (int y = 0; y < kernel_logger::COLUMN_CHARS - 1; y++) {
		for (int x = 0; x < kernel_logger::ROW_CHARS; x++) {
			kscreen->draw_string(Point2D{ adjusted_x(x), adjusted_y(y) },
								 buffer_[y][x], font_color_.GetCode());
		}
	}

	cursor_x_ = 0;
	cursor_y_ = kernel_logger::COLUMN_CHARS - 1;
}

kernel_logger* klogger;
alignas(kernel_logger) char kernel_logger_buf[sizeof(kernel_logger)];

void initialize_kernel_logger()
{
	klogger = new (kernel_logger_buf) kernel_logger{ Color{ 255, 255, 255 } };
}
