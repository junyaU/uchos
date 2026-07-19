#include "screen.hpp"
#include <cstdint>
#include <new> // NOLINT(misc-include-cleaner)
#include "../../UchLoaderPkg/frame_buffer_conf.hpp"
#include "color.hpp"
#include "font.hpp"
#include "graphics/log_sink.hpp"
#include "log/log.hpp"
#include "point2d.hpp"

namespace kernel::graphics
{

Screen::Screen(const FrameBufferConf& frame_buffer_conf, Color bg_color)
	: pixels_per_scan_line_(frame_buffer_conf.pixels_per_scan_line),
	  horizontal_resolution_(frame_buffer_conf.horizontal_resolution),
	  vertical_resolution_(frame_buffer_conf.vertical_resolution),
	  bg_color_(bg_color),
	  frame_buffer_(reinterpret_cast<uint32_t*>(frame_buffer_conf.frame_buffer))
{
}

void Screen::put_pixel(Point2D point, uint32_t color_code)
{
	const int x = point.GetX();
	const int y = point.GetY();

	// Clip silently: logging here would recurse back into put_pixel
	if (x < 0 || y < 0 || x >= static_cast<int>(horizontal_resolution_) ||
		y >= static_cast<int>(vertical_resolution_)) {
		return;
	}

	frame_buffer_[pixels_per_scan_line_ * y + x] = color_code;
}

void Screen::fill_rectangle(Point2D position,
							Point2D size,
							const uint32_t color_code)
{
	for (int dy = 0; dy < size.GetY(); dy++) {
		for (int dx = 0; dx < size.GetX(); dx++) {
			put_pixel(position + Point2D{ dx, dy }, color_code);
		}
	}
}

Screen* kscreen;
alignas(Screen) char screen_buffer[sizeof(Screen)];

void initialize(const FrameBufferConf& frame_buffer_conf, Color bg_color)
{
	kscreen = new (screen_buffer) Screen{ frame_buffer_conf, bg_color };

	kscreen->fill_rectangle(Point2D{ 0, 0 }, kscreen->size(),
							kscreen->bg_color().code());

	// From here on log lines can be drawn; everything logged before this
	// point exists on the serial console only.
	kernel::log::register_screen_sink(screen_log_sink);
}

} // namespace kernel::graphics
