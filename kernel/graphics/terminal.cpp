#include "terminal.hpp"
#include "../command_line/controller.hpp"
#include "../task/task.hpp"
#include "color.hpp"
#include "font.hpp"
#include "screen.hpp"
#include <cstdint>
#include <cstring>
#include <memory>

terminal::terminal(Color font_color, const char* user_name, Color user_name_color)
	: font_color_{ font_color }, user_name_color_{ user_name_color }
{
	memset(user_name_, '\0', sizeof(user_name_));
	memcpy(user_name_, user_name, strlen(user_name));
	clear();

	print("Hello, uchos\n");

	show_user_name();
}

void terminal::print(const char* s)
{
	while (*s != '\0') {
		if (cursor_x_ == ROW_CHARS - 1) {
			cursor_x_ = 0;
		} else if (cursor_x_ < ROW_CHARS - 1) {
			cursor_x_++;
		}

		if (*s == '\n') {
			next_line();
			s++;
			continue;
		}

		const int current_x = cursor_x_ == 0 ? ROW_CHARS - 1 : cursor_x_ - 1;
		const int target_x_position = adjusted_x(
				cursor_y_ == 0 ? current_x : current_x + user_name_length());

		buffer_[cursor_y_][current_x] = *s;
		kscreen->fill_rectangle({ target_x_position, adjusted_y(cursor_y_) },
								kfont->size(), kscreen->bg_color().GetCode());
		kscreen->draw_string({ target_x_position, adjusted_y(cursor_y_) }, *s,
							 font_color_.GetCode());

		if (cursor_x_ == 0) {
			next_line();
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

void terminal::print_interrupt_hex(uint64_t value)
{
	print("0x");

	for (int i = 0; i < 16; ++i) {
		char buf[2] = { '0', '\0' };
		int x = (value >> (4 * (15 - i))) & 0xfU;
		char c = x < 10 ? '0' + x : 'a' + x - 10;
		buf[0] = c;
		print(buf);
	}

	print("\n");
}

void terminal::input_key(uint8_t c)
{
	if (c == '\n') {
		char command[100];
		memcpy(command, buffer_[cursor_y_], sizeof(buffer_[cursor_y_]));

		kscreen->fill_rectangle(
				{ adjusted_x(user_name_length()), adjusted_y(cursor_y_) },
				{ kfont->width() * (cursor_x_ + 1), kfont->height() },
				kscreen->bg_color().GetCode());

		cursor_x_ = 0;
		memset(buffer_[cursor_y_], '\0', sizeof(buffer_[cursor_y_]));

		cl_ctrl_->process_command(command, *this);
		next_line();

		return;
	}

	const uint8_t delete_key = 0x08;

	if (c != delete_key) {
		printf("%c", c);
		return;
	}

	if (cursor_x_ == 0) {
		return;
	}

	kscreen->fill_rectangle(
			{ adjusted_x(cursor_x_ + user_name_length()), adjusted_y(cursor_y_) },
			kfont->size(), kscreen->bg_color().GetCode());

	--cursor_x_;
	buffer_[cursor_y_][cursor_x_] = '\0';

	kscreen->fill_rectangle(
			{ adjusted_x(cursor_x_ + user_name_length()), adjusted_y(cursor_y_) },
			kfont->size(), kscreen->bg_color().GetCode());
}

void terminal::cursor_blink()
{
	const Color cursor_color = cursor_visible_ ? kscreen->bg_color() : font_color_;

	kscreen->fill_rectangle(
			{ adjusted_x(cursor_x_ + user_name_length()), adjusted_y(cursor_y_) },
			kfont->size(), cursor_color.GetCode());

	cursor_visible_ = !cursor_visible_;
}

void terminal::clear()
{
	for (int y = 0; y < terminal::COLUMN_CHARS; y++) {
		memset(buffer_[y], '\0', sizeof(buffer_[y]));
	}

	cursor_x_ = 0;
	cursor_y_ = 0;

	kscreen->fill_rectangle({ 0, 0 },
							{ terminal::ROW_CHARS * kfont->width() + START_X,
							  terminal::COLUMN_CHARS * kfont->height() +
									  (terminal::COLUMN_CHARS * LINE_SPACING) +
									  START_Y },
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
			kscreen->draw_string({ adjusted_x(x), adjusted_y(cursor_y_) },
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
			kscreen->draw_string({ adjusted_x(x), adjusted_y(y) }, buffer_[y][x],
								 font_color_.GetCode());
		}
	}

	cursor_x_ = 0;
	cursor_y_ = terminal::COLUMN_CHARS - 1;
}

void terminal::show_user_name()
{
	kscreen->draw_string({ adjusted_x(cursor_x_), adjusted_y(cursor_y_) },
						 user_name_, user_name_color_.GetCode());
	kscreen->draw_string(
			{ adjusted_x(cursor_x_ + strlen(user_name_)), adjusted_y(cursor_y_) },
			":~$ ", font_color_.GetCode());
}

void terminal::initialize_command_line()
{
	cl_ctrl_ = std::make_unique<command_line::controller>();
}

terminal* main_terminal;
alignas(terminal) char kernel_logger_buf[sizeof(terminal)];

void initialize_terminal()
{
	main_terminal = new (kernel_logger_buf)
			terminal{ Color{ 255, 255, 255 }, "root@uchos", { 74, 246, 38 } };
}

void task_terminal()
{
	task* t = CURRENT_TASK;

	main_terminal->set_task_id(t->id);

	t->message_handlers[NOTIFY_KEY_INPUT] = [](const message& m) {
		if (m.data.key_input.press != 0) {
			main_terminal->input_key(m.data.key_input.ascii);
		}
	};

	t->message_handlers[NOTIFY_CURSOR_BLINK] = [](const message& m) {
		main_terminal->cursor_blink();
	};

	process_messages(t);
}
