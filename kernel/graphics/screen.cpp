#include "screen.hpp"
#include "../../UchLoaderPkg/frame_buffer_conf.hpp"
#include "color.hpp"
#include "font.hpp"
#include <cstdint>
#include <new>

screen::screen(const FrameBufferConf& frame_buffer_conf, Color bg_color)
	: pixels_per_scan_line_(frame_buffer_conf.pixels_per_scan_line),
	  horizontal_resolution_(frame_buffer_conf.horizontal_resolution),
	  vertical_resolution_(frame_buffer_conf.vertical_resolution),
	  bg_color_(bg_color),
	  frame_buffer_(reinterpret_cast<uint32_t*>(frame_buffer_conf.frame_buffer))
{
}

void screen::put_pixel(Point2D point, uint32_t color_code)
{
	const uint64_t pixel_position =
			pixels_per_scan_line_ * point.GetY() + point.GetX();
	frame_buffer_[pixel_position] = color_code;
}

void screen::fill_rectangle(Point2D position,
							Point2D size,
							const uint32_t color_code)
{
	for (int dy = 0; dy < size.GetY(); dy++) {
		for (int dx = 0; dx < size.GetX(); dx++) {
			put_pixel(position + Point2D{ dx, dy }, color_code);
		}
	}
}

screen* kscreen;
alignas(screen) char screen_buffer[sizeof(screen)];

void initialize_screen(const FrameBufferConf& frame_buffer_conf, Color bg_color)
{
	kscreen = new (screen_buffer) screen{ frame_buffer_conf, bg_color };

	kscreen->fill_rectangle(Point2D{ 0, 0 }, kscreen->size(),
							kscreen->bg_color().GetCode());
}
