#pragma once

#include <cstdint>

#include "../UchLoaderPkg/frame_buffer_conf.hpp"
#include "color.hpp"
#include "font.hpp"
#include "point2d.hpp"

class Screen
{
public:
	Screen(const FrameBufferConf& frame_buffer_conf,
		   Color bg_color,
		   Color taskbar_color);
	int Width() const { return horizontal_resolution_; }
	int Height() const { return vertical_resolution_; }
	Point2D Size() const { return { Width(), Height() }; }

	Color BgColor() const { return bg_color_; }
	Color TaskbarColor() const { return taskbar_color_; }

	void PutPixel(Point2D point, const uint32_t color_code);
	void FillRectangle(Point2D position, Point2D size, const uint32_t color_code);
	void DrawString(Point2D position, const char* s, const uint32_t color_code);
	void DrawString(Point2D position, char s, const uint32_t color_code);

private:
	uint64_t pixels_per_scan_line_;
	uint64_t horizontal_resolution_;
	uint64_t vertical_resolution_;
	Color bg_color_;
	Color taskbar_color_;
	uint32_t* frame_buffer_;
};

extern Screen* screen;

void InitializeScreen(const FrameBufferConf& frame_buffer_conf,
					  Color bg_color,
					  Color taskbar_color);

void DrawTimer(const char* s);
