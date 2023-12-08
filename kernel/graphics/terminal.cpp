#include "terminal.hpp"
#include "color.hpp"
#include "font.hpp"
#include "screen.hpp"
#include <cstring>
#include <new>

terminal::terminal(Color font_color, const char* user_name, Color user_name_color)
	: font_color_{ font_color }, user_name_color_{ user_name_color }
{
	memset(user_name_, '\0', sizeof(user_name_));
	memcpy(user_name_, user_name, strlen(user_name));
	clear();
}

void terminal::print(const char* s)
{
	while (*s != '\0') {
		if (*s == '\n') {
			next_line();
			s++;
			continue;
		}

		buffer_[cursor_y_][cursor_x_] = *s;
		kscreen->draw_string(Point2D{ adjusted_x(cursor_x_ + strlen(user_name_) + 4),
									  adjusted_y(cursor_y_) },
							 *s, font_color_.GetCode());

		if (cursor_x_ == ROW_CHARS - 1) {
			next_line();
		} else if (cursor_x_ < ROW_CHARS - 1) {
			cursor_x_++;
		}

		s++;
	}
}

void terminal::info(const char* s)
{
	print("[INFO] ");
	print(s);
	print("\n");
}

void terminal::error(const char* s)
{
	print("[ERROR] ");
	print(s);
	print("\n");
}

void terminal::input_key(uint8_t c)
{
	const uint8_t delete_key = 0x08;

	if (c != delete_key) {
		printf("%c", c);
		return;
	}

	if (cursor_x_ == 0) {
		return;
	}

	--cursor_x_;
	buffer_[cursor_y_][cursor_x_] = '\0';

	kscreen->fill_rectangle(Point2D{ adjusted_x(cursor_x_ + strlen(user_name_) + 4),
									 adjusted_y(cursor_y_) },
							Point2D{ adjusted_x(cursor_x_), adjusted_y(cursor_y_) },
							kscreen->bg_color().GetCode());
}

void terminal::clear()
{
	for (int y = 0; y < terminal::COLUMN_CHARS; y++) {
		memset(buffer_[y], '\0', sizeof(buffer_[y]));
	}

	cursor_x_ = 0;
	cursor_y_ = 0;

	kscreen->fill_rectangle(
			Point2D{ 0, 0 },
			Point2D{ terminal::ROW_CHARS * kfont->width() + START_X,
					 terminal::COLUMN_CHARS * kfont->height() +
							 (terminal::COLUMN_CHARS * LINE_SPACING) + START_Y },
			kscreen->bg_color().GetCode());
}

void terminal::next_line()
{
	if (cursor_y_ == COLUMN_CHARS - 1) {
		scroll_lines();
	} else {
		kscreen->fill_rectangle(
				Point2D{ 0, adjusted_y(cursor_y_) },
				Point2D{ adjusted_x(ROW_CHARS), adjusted_y(cursor_y_) },
				kscreen->bg_color().GetCode());

		for (int x = 0; x < ROW_CHARS; x++) {
			kscreen->draw_string(Point2D{ adjusted_x(x), adjusted_y(cursor_y_) },
								 buffer_[cursor_y_][x], font_color_.GetCode());
		}

		cursor_x_ = 0;
		cursor_y_++;
	}

	show_user_name();
}

void terminal::scroll_lines()
{
	kscreen->fill_rectangle(
			Point2D{ 0, 0 },
			Point2D{ adjusted_x(ROW_CHARS), adjusted_y(COLUMN_CHARS) },
			kscreen->bg_color().GetCode());

	memcpy(buffer_[0], buffer_[1], sizeof(buffer_) - sizeof(buffer_[0]));

	for (int x = 0; x < terminal::ROW_CHARS; x++) {
		buffer_[terminal::COLUMN_CHARS - 1][x] = '\0';
	}

	for (int y = 0; y < terminal::COLUMN_CHARS - 1; y++) {
		for (int x = 0; x < terminal::ROW_CHARS; x++) {
			kscreen->draw_string(Point2D{ adjusted_x(x), adjusted_y(y) },
								 buffer_[y][x], font_color_.GetCode());
		}
	}

	cursor_x_ = 0;
	cursor_y_ = terminal::COLUMN_CHARS - 1;
}

void terminal::show_user_name()
{
	kscreen->draw_string(Point2D{ adjusted_x(cursor_x_), adjusted_y(cursor_y_) },
						 user_name_, user_name_color_.GetCode());
	kscreen->draw_string(Point2D{ adjusted_x(cursor_x_ + strlen(user_name_)),
								  adjusted_y(cursor_y_) },
						 ":~$ ", font_color_.GetCode());
}

terminal* main_terminal;
alignas(terminal) char kernel_logger_buf[sizeof(terminal)];

void initialize_kernel_logger()
{
	main_terminal = new (kernel_logger_buf)
			terminal{ Color{ 255, 255, 255 }, "root@uchos", { 74, 246, 38 } };
}
