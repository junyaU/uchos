#include "graphics/log.hpp"
#include <cstdarg>
#include <cstddef>
#include <cstdio>
#include <cstring>
#include "graphics/font.hpp"
#include "graphics/screen.hpp"
#include "hardware/serial.hpp"

namespace
{
kernel::graphics::LogLevel current_log_level = kernel::graphics::LogLevel::ERROR;
int kernel_cursor_x = 0;
int kernel_cursor_y = 5;
} // namespace

namespace kernel::graphics
{

void change_log_level(LogLevel level) { current_log_level = level; }

} // namespace kernel::graphics

void printk(kernel::graphics::LogLevel level, const char* format, ...)
{
	if (format == nullptr) {
		return;
	}

	va_list ap;
	char s[1024];

	va_start(ap, format);
	vsnprintf(s, sizeof(s), format, ap);
	va_end(ap);

	// The serial port receives every message regardless of the on-screen
	// log level: it is the only output visible in headless runs and serves
	// as a full boot trace when debugging CI failures.
	kernel::hw::serial::write_string(s);
	kernel::hw::serial::write_string("\n");

	// The screen shows only messages that match the current log level.
	if (level != current_log_level) {
		return;
	}

	for (size_t i = 0; i < strlen(s) && s[i] != '\0'; ++i) {
		kernel::graphics::write_ascii(
				*kernel::graphics::kscreen,
				{ kernel_cursor_x++ * 8, kernel_cursor_y * 16 }, s[i], 0xffff00);

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
