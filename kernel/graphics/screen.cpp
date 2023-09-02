#include "screen.hpp"

#include <new>

#include "color.hpp"
#include "font.hpp"

Screen::Screen(const FrameBufferConf& frame_buffer_conf,
			   Color bg_color,
			   Color taskbar_color)
	: pixels_per_scan_line_(frame_buffer_conf.pixels_per_scan_line),
	  horizontal_resolution_(frame_buffer_conf.horizontal_resolution),
	  vertical_resolution_(frame_buffer_conf.vertical_resolution),
	  bg_color_(bg_color),
	  taskbar_color_(taskbar_color),
	  frame_buffer_(reinterpret_cast<uint32_t*>(frame_buffer_conf.frame_buffer))
{
}

void Screen::PutPixel(Point2D point, const uint32_t color_code)
{
	const uint64_t pixel_position =
			pixels_per_scan_line_ * point.GetY() + point.GetX();
	frame_buffer_[pixel_position] = color_code;
}

void Screen::FillRectangle(Point2D position, Point2D size, const uint32_t color_code)
{
	for (int dy = 0; dy < size.GetY(); dy++) {
		for (int dx = 0; dx < size.GetX(); dx++) {
			PutPixel(position + Point2D{ dx, dy }, color_code);
		}
	}
}

void Screen::DrawString(Point2D position, const char* s, const uint32_t color_code)
{
	int font_position = 0;
	while (*s) {
		const uint8_t* font = bitmap_font->GetFont(*s);
		if (font) {
			for (int dy = 0; dy < bitmap_font->Height(); dy++) {
				for (int dx = 0; dx < bitmap_font->Width(); dx++) {
					if (font[dy] << dx & 0x80) {
						PutPixel(position +
										 Point2D{ font_position * bitmap_font
																		  ->Width() +
														  dx,
												  dy },
								 color_code);
					}
				}
			}
		}

		font_position++;
		s++;
	}
}

void Screen::DrawString(Point2D position, char s, const uint32_t color_code)
{
	char temp[2] = { s, '\0' };
	DrawString(position, temp, color_code);
}

Screen* screen;
char screen_buffer[sizeof(Screen)];

void InitializeScreen(const FrameBufferConf& frame_buffer_conf,
					  Color bg_color,
					  Color taskbar_color)
{
	screen =
			new (screen_buffer) Screen{ frame_buffer_conf, bg_color, taskbar_color };

	screen->FillRectangle(Point2D{ 0, 0 }, screen->Size(),
						  screen->BgColor().GetCode());

	int taskbar_height = screen->Height() * 0.08;
	screen->FillRectangle({ 0, screen->Height() - taskbar_height },
						  { screen->Width(), taskbar_height },
						  screen->TaskbarColor().GetCode());
}

void DrawTimer(const char* s)
{
	Point2D draw_area = { 0, static_cast<int>(screen->Height() -
											  screen->Height() * 0.08) };

	screen->FillRectangle(draw_area,
						  { bitmap_font->Width() * 8, bitmap_font->Height() },
						  screen->TaskbarColor().GetCode());

	screen->DrawString(draw_area, s, 0xffffff);
}