#pragma once

struct FrameBufferConf;

#include "color.hpp"
#include "point2d.hpp"
#include <cstdint>

class screen
{
public:
	screen(const FrameBufferConf& frame_buffer_conf, Color bg_color);

	int width() const { return horizontal_resolution_; }
	int height() const { return vertical_resolution_; }
	point2d size() const { return { width(), height() }; }

	Color bg_color() const { return bg_color_; }

	void put_pixel(point2d point, uint32_t color_code);
	void fill_rectangle(point2d position, point2d size, uint32_t color_code);

private:
	uint64_t pixels_per_scan_line_;
	uint64_t horizontal_resolution_;
	uint64_t vertical_resolution_;
	Color bg_color_;
	uint32_t* frame_buffer_;
};

extern screen* kscreen;

void initialize_screen(const FrameBufferConf& frame_buffer_conf, Color bg_color);
