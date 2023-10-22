#pragma once

#include <cstdint>

#include "../point2d.hpp"

class BitmapFont
{
public:
	BitmapFont(int width, int height);
	const uint8_t* GetFont(char c);
	int Width() const { return width_; }
	int Height() const { return height_; }

private:
	const uint8_t* font_data_;
	int width_;
	int height_;
};

extern BitmapFont* bitmap_font;

void InitializeFont();