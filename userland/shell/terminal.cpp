#include "terminal.hpp"
#include <../../libs/user/print.hpp>
#include <cstring>

terminal::terminal()
{
	char user_name[16] = "root@uchos";
	memcpy(this->user_name, user_name, 16);

	cursor_x = 0;
	cursor_y = 0;
	for (int i = 0; i < 30; i++) {
		for (int j = 0; j < 98; j++) {
			buffer[i][j] = '\0';
		}
	}
}

void terminal::print_user()
{
	print_text(cursor_x, cursor_y, user_name, 0x00ff00);
	cursor_x += strlen(user_name);
	print_text(cursor_x, cursor_y, ":~$ ", 0xffffff);
	cursor_x += 4;
}

void terminal::print(const char* s)
{
	print_text(cursor_x, cursor_y, s, 0xffffff);
	cursor_x += strlen(s);
	if (cursor_x == 98) {
		cursor_x = 0;
		cursor_y += 1;
	}
	if (cursor_y == 30) {
		cursor_y = 0;
	}
}
