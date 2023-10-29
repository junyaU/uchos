#pragma once

struct FrameBufferConf;

#include "../point2d.hpp"
#include "color.hpp"
#include <cstdint>

class screen
{
public:
	screen(const FrameBufferConf& frame_buffer_conf, Color bg_color);

	int width() const { return horizontal_resolution_; }
	int height() const { return vertical_resolution_; }
	Point2D size() const { return { width(), height() }; }

	Color bg_color() const { return bg_color_; }

	void put_pixel(Point2D point, uint32_t color_code);
	void fill_rectangle(Point2D position, Point2D size, uint32_t color_code);
	void draw_string(Point2D position, const char* s, uint32_t color_code);
	void draw_string(Point2D position, char s, uint32_t color_code);

private:
	uint64_t pixels_per_scan_line_;
	uint64_t horizontal_resolution_;
	uint64_t vertical_resolution_;
	Color bg_color_;
	uint32_t* frame_buffer_;
};

extern screen* kscreen;

void initialize_screen(const FrameBufferConf& frame_buffer_conf, Color bg_color);

void draw_timer(const char* s);
