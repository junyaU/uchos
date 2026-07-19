/**
 * @file log_sink.cpp
 * @brief On-screen rendering of kernel log lines
 */

#include "graphics/log_sink.hpp"
#include <cstddef>
#include <cstring>
#include "graphics/font.hpp"
#include "graphics/screen.hpp"

namespace
{
int kernel_cursor_x = 0;
int kernel_cursor_y = 5;

constexpr int GLYPH_WIDTH = 8;
constexpr int GLYPH_HEIGHT = 16;

// Advance the on-screen cursor to the next line, wrapping back to the top
// when the bottom of the screen is reached. The kernel log used to keep
// incrementing the row forever and write past the frame buffer
// (issue #313); the serial log remains the full, unwrapped record.
void advance_line()
{
	kernel_cursor_x = 0;
	++kernel_cursor_y;

	auto* screen = kernel::graphics::kscreen;
	const int rows = screen->height() / GLYPH_HEIGHT;
	if (kernel_cursor_y >= rows) {
		kernel_cursor_y = 0;
	}

	// Erase whatever was on the line we are about to write
	screen->fill_rectangle({ 0, kernel_cursor_y * GLYPH_HEIGHT },
						   { screen->width(), GLYPH_HEIGHT },
						   screen->bg_color().code());
}
} // namespace

namespace kernel::graphics
{

void screen_log_sink(const char* line)
{
	for (size_t i = 0; i < strlen(line) && line[i] != '\0'; ++i) {
		write_ascii(
				*kscreen,
				{ kernel_cursor_x++ * GLYPH_WIDTH, kernel_cursor_y * GLYPH_HEIGHT },
				line[i], 0xffff00);

		if (line[i] == '\n') {
			advance_line();
			continue;
		}

		if (kernel_cursor_x >= 98) {
			advance_line();
		}
	}

	advance_line();
}

} // namespace kernel::graphics
