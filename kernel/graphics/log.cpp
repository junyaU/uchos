#include "graphics/log.hpp"
#include "graphics/font.hpp"
#include "graphics/screen.hpp"
#include <cstddef>
#include <cstring>
#include <libs/common/message.hpp>
#include <libs/common/types.hpp>

namespace
{
kernel::graphics::log_level CURRENT_LOG_LEVEL = kernel::graphics::log_level::ERROR;
int kernel_cursor_x = 0;
int kernel_cursor_y = 5;
} // namespace

namespace kernel::graphics {

void change_log_level(log_level level) { CURRENT_LOG_LEVEL = level; }

} // namespace kernel::graphics

void printk(kernel::graphics::log_level level, const char* format, ...)
{
	if (level != CURRENT_LOG_LEVEL || format == nullptr) {
		return;
	}

	va_list ap;
	char s[1024];

	va_start(ap, format);
	vsnprintf(s, sizeof(s), format, ap);
	va_end(ap);

	for (size_t i = 0; i < strlen(s) && s[i] != '\0'; ++i) {
		kernel::graphics::write_ascii(*kscreen, { kernel_cursor_x++ * 8, kernel_cursor_y * 16 }, s[i],
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
