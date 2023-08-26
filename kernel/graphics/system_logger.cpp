#include "system_logger.hpp"

#include <cstdio>

#include "font.hpp"
#include "screen.hpp"

SystemLogger::SystemLogger(Color font_color) : font_color_{ font_color } { Clear(); }

void SystemLogger::Print(const char* s)
{
	while (*s) {
		if (*s == '\n') {
			NextLine();
			s++;
			continue;
		}

		buffer_[cursor_y_][cursor_x_] = *s;
		screen->DrawString(Point2D{ cursor_x_ * bitmap_font->Width(),
									cursor_y_ * bitmap_font->Height() },
						   *s, font_color_.GetCode());

		if (cursor_x_ == kCharsPerLine - 1) {
			NextLine();
		} else if (cursor_x_ < kCharsPerLine - 1) {
			cursor_x_++;
		}

		s++;
	}
}

void SystemLogger::Printf(const char* format, ...)
{
	char s[1024];

	va_list ap;
	va_start(ap, format);
	vsprintf(s, format, ap);
	va_end(ap);

	Print(s);
}

void SystemLogger::Clear()
{
	for (int y = 0; y < SystemLogger::kLines; y++) {
		memset(buffer_[y], '\0', sizeof(buffer_[y]));
	}

	cursor_x_ = 0;
	cursor_y_ = 0;

	screen->FillRectangle(
			Point2D{ 0, 0 },
			Point2D{ SystemLogger::kCharsPerLine * bitmap_font->Width(),
					 SystemLogger::kLines * bitmap_font->Height() },
			screen->BgColor().GetCode());
}

void SystemLogger::NextLine()
{
	if (cursor_y_ == kLines - 1) {
		ScrollLines();
	} else {
		cursor_x_ = 0;
		cursor_y_++;
	}
}

void SystemLogger::ScrollLines()
{
	screen->FillRectangle(
			Point2D{ 0, 0 },
			Point2D{ SystemLogger::kCharsPerLine * bitmap_font->Width(),
					 SystemLogger::kLines * bitmap_font->Height() },
			screen->BgColor().GetCode());

	memcpy(buffer_[0], buffer_[1], sizeof(buffer_) - sizeof(buffer_[0]));

	for (int x = 0; x < SystemLogger::kCharsPerLine; x++) {
		buffer_[SystemLogger::kLines - 1][x] = '\0';
	}

	for (int y = 0; y < SystemLogger::kLines - 1; y++) {
		for (int x = 0; x < SystemLogger::kCharsPerLine; x++) {
			screen->DrawString(
					Point2D{ x * bitmap_font->Width(), y * bitmap_font->Height() },
					buffer_[y][x], font_color_.GetCode());
		}
	}

	cursor_x_ = 0;
	cursor_y_ = SystemLogger::kLines - 1;
}

SystemLogger* system_logger;
char system_logger_buf[sizeof(SystemLogger)];

void InitializeSystemLogger()
{
	system_logger = new (system_logger_buf) SystemLogger{ Color{ 255, 255, 255 } };
}
