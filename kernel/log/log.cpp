/**
 * @file log.cpp
 * @brief Kernel logging: serial always, screen via registered sink
 */

#include "log/log.hpp"
#include <cstdarg>
#include <cstdio>
#include "hardware/serial.hpp"

namespace
{
kernel::log::LogLevel current_log_level = kernel::log::LogLevel::ERROR;
kernel::log::ScreenSink screen_sink = nullptr;
} // namespace

namespace kernel::log
{

void change_log_level(LogLevel level) { current_log_level = level; }

void register_screen_sink(ScreenSink sink) { screen_sink = sink; }

} // namespace kernel::log

void printk(kernel::log::LogLevel level, const char* format, ...)
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

	// The screen shows only messages that match the selected level, and
	// only once graphics has registered its sink -- before that point
	// (early boot) output exists on the serial line only.
	if (level != current_log_level || screen_sink == nullptr) {
		return;
	}

	screen_sink(s);
}
