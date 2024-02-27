#include "terminal.hpp"
#include "../hardware/keyboard.hpp"
#include "../shell/controller.hpp"
#include "../task/ipc.hpp"
#include "../task/task.hpp"
#include "../types.hpp"
#include "color.hpp"
#include "font.hpp"
#include "screen.hpp"
#include <cstddef>
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

void terminal::initialize_fds()
{
	for (int i = 0; i < fds_.size(); i++) {
		fds_[i] = std::make_shared<term_file_descriptor>();
	}
}

void terminal::put_char(char32_t c, int size, Color color)
{
	if (cursor_x_ == ROW_CHARS - 1) {
		cursor_x_ = 0;
	} else if (cursor_x_ < ROW_CHARS - 1) {
		cursor_x_++;
	}

	if (c == U'\n') {
		next_line();
		return;
	}

	const int current_x = cursor_x_ == 0 ? ROW_CHARS - 1 : cursor_x_ - 1;
	const int target_x_position =
			adjusted_x(cursor_y_ == 0 ? current_x : current_x + user_name_length());

	buffer_[cursor_y_][current_x] = c;
	kscreen->fill_rectangle({ target_x_position, adjusted_y(cursor_y_) },
							kfont->size(), kscreen->bg_color().GetCode());

	write_unicode(*kscreen, { target_x_position, adjusted_y(cursor_y_) }, c,
				  color.GetCode());

	if (size > 1) {
		cursor_x_++;
	}

	if (cursor_x_ == ROW_CHARS - 1) {
		next_line();
	}
}

size_t terminal::print(const char* s, Color color)
{
	const char* start = s;
	while (*s != '\0') {
		const int size = utf8_size(static_cast<uint8_t>(*s));
		put_char(utf8_to_unicode(s), size, color);

		s += size;
	}

	return s - start;
}

size_t terminal::print(const char* s, size_t len, Color color)
{
	const char* start = s;
	for (size_t i = 0; i < len; ++i) {
		const int size = utf8_size(static_cast<uint8_t>(s[i]));
		put_char(utf8_to_unicode(&s[i]), size, color);
		s += size;
	}

	return s - start;
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
		const int x = (value >> (4 * (15 - i))) & 0xfU;
		const char c = x < 10 ? '0' + x : 'a' + x - 10;
		buf[0] = c;
		print(buf);
	}

	print("\n");
}

void terminal::input_key(uint8_t c)
{
	if (c == '\n') {
		char buffer[100];
		for (int i = 0; i < 100; i++) {
			buffer[i] = decode_utf8(buffer_[cursor_y_][i]);
		}

		char command[100];

		memcpy(command, buffer, sizeof(buffer));

		kscreen->fill_rectangle(
				{ adjusted_x(user_name_length()), adjusted_y(cursor_y_) },
				{ kfont->width() * (cursor_x_ + 1), kfont->height() },
				kscreen->bg_color().GetCode());

		cursor_x_ = 0;
		memset(&buffer_[cursor_y_], '\0', sizeof(buffer_[cursor_y_]));

		shell_->process_command(command, *this);
		next_line();

		return;
	}

	const uint8_t delete_key = 0x08;

	if (c != delete_key) {
		print(reinterpret_cast<const char*>(&c), 1);
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
		memset(&buffer_[y], '\0', sizeof(buffer_[y]));
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
			write_unicode(*kscreen, { adjusted_x(x), adjusted_y(cursor_y_) },
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

	memcpy(&buffer_, &buffer_[1], sizeof(buffer_) - sizeof(buffer_[0]));

	for (int x = 0; x < terminal::ROW_CHARS; x++) {
		buffer_[terminal::COLUMN_CHARS - 1][x] = '\0';
	}

	for (int y = 0; y < terminal::COLUMN_CHARS - 1; y++) {
		for (int x = 0; x < terminal::ROW_CHARS; x++) {
			write_unicode(*kscreen, { adjusted_x(x), adjusted_y(y) }, buffer_[y][x],
						  font_color_.GetCode());
		}
	}

	cursor_x_ = 0;
	cursor_y_ = terminal::COLUMN_CHARS - 1;
}

void terminal::show_user_name()
{
	write_string(*kscreen, { adjusted_x(cursor_x_), adjusted_y(cursor_y_) },
				 user_name_, user_name_color_.GetCode());
	write_string(
			*kscreen,
			{ adjusted_x(cursor_x_ + strlen(user_name_)), adjusted_y(cursor_y_) },
			":~$ ", font_color_.GetCode());
}

void terminal::initialize_command_line()
{
	shell_ = std::make_unique<shell::controller>();
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

	t->message_handlers[NOTIFY_KEY_INPUT] = +[](const message& m) {
		main_terminal->input_key(m.data.key_input.ascii);
	};

	t->message_handlers[NOTIFY_CURSOR_BLINK] =
			+[](const message& m) { main_terminal->cursor_blink(); };

	t->message_handlers[NOTIFY_WRITE] = +[](const message& m) {
		switch (m.data.write.level) {
			case KERN_DEBUG:
				main_terminal->print(m.data.write.s);
				break;
			case KERN_ERROR:
				main_terminal->error(m.data.write.s);
				break;
			case KERN_INFO:
				main_terminal->info(m.data.write.s);
				break;
		}
	};

	process_messages(t);
}

size_t term_file_descriptor::read(void* buf, size_t len)
{
	char* bufc = reinterpret_cast<char*>(buf);
	task* t = CURRENT_TASK;

	while (true) {
		if (t->messages.empty()) {
			t->state = TASK_WAITING;
			continue;
		}

		t->state = TASK_RUNNING;

		const message m = t->messages.front();
		t->messages.pop();
		if (m.type != NOTIFY_KEY_INPUT) {
			continue;
		}

		if (is_EOT(m.data.key_input.modifier, m.data.key_input.key_code)) {
			main_terminal->print("^D");
			return 0;
		}

		bufc[0] = m.data.key_input.ascii;
		main_terminal->print(bufc);
		return 1;
	}
}

size_t term_file_descriptor::write(const void* buf, size_t len)
{
	printk(KERN_DEBUG, reinterpret_cast<const char*>(buf));
	return len;
}
