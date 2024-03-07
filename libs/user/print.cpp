#include "print.hpp"
#include "syscall.hpp"

void print_text(int x, int y, const char* text, uint32_t color)
{

	sys_draw_text(text, x, y, color);
}
