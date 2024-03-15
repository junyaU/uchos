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
	sys_fill_rect(x, y, FONT_WIDTH, FONT_HEIGHT, color);
}

void clear_screen() { sys_fill_rect(0, 0, 800, 600, 0x000000); }
