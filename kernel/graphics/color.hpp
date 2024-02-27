#pragma once

#include <cstdint>

const int kBytesPerPixel = 4;

class Color
{
public:
	Color(uint8_t r, uint8_t g, uint8_t b);
	uint32_t GetCode() const;

private:
	uint8_t r_;
	uint8_t g_;
	uint8_t b_;
};

static Color WHITE = Color(255, 255, 255);
static Color LIGHT_GRAY = Color(192, 192, 192);
static Color GRAY = Color(128, 128, 128);
static Color DARK_GRAY = Color(64, 64, 64);
static Color BLACK = Color(0, 0, 0);
static Color RED = Color(255, 0, 0);
static Color GREEN = Color(0, 255, 0);
static Color BLUE = Color(0, 0, 255);
