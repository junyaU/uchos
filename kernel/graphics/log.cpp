#include "log.hpp"
#include "font.hpp"
#include "screen.hpp"
#include <cstddef>
#include <cstring>
#include <libs/common/message.hpp>
#include <libs/common/types.hpp>

void printk(int level, const char* format, ...)
{
	if (level != CURRENT_LOG_LEVEL) {
		return;
	}

	va_list ap;
	char s[1024];

	va_start(ap, format);
	vsprintf(s, format, ap);
	va_end(ap);

	for (size_t i = 0; i < strlen(s) && s[i] != '\0'; ++i) {
		write_ascii(*kscreen, { kernel_cursor_x++ * 8, kernel_cursor_y * 16 }, s[i],
					0xffff00);

		if (s[i] == '\n') {
			kernel_cursor_x = 0;
			++kernel_cursor_y;
			continue;
		}

		if (kernel_cursor_x >= 98) {
			kernel_cursor_x = 0;
			++kernel_cursor_y;
		}
	}

	kernel_cursor_x = 0;
	++kernel_cursor_y;
}
