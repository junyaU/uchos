#pragma once

#include <cstdint>

#include "../point2d.hpp"

class bitmap_font
{
public:
	bitmap_font(int width, int height);
	const uint8_t* get_font(char c);
	int width() const { return width_; }
	int height() const { return height_; }

private:
	const uint8_t* font_data_;
	int width_;
	int height_;
};

extern bitmap_font* kfont;

void initialize_font();