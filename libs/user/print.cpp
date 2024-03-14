#include "print.hpp"
#include "syscall.hpp"
#include <cstdint>

void print_text(int x, int y, const char* text, uint32_t color)
{
	sys_draw_text(text, x, y, color);
}

void delete_char(int x, int y)
{
	const uint32_t color = 0x000000;
	const int font_width = 8;
	const int font_height = 16;
	sys_fill_rect(x * font_width, y * font_height, font_width, font_height, color);
}

void clear_screen() { sys_fill_rect(0, 0, 800, 600, 0x000000); }
