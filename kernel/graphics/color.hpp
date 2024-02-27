#pragma once

#include <cstdint>

constexpr int kBytesPerPixel = 4;

class Color
{
public:
	constexpr Color(uint8_t r, uint8_t g, uint8_t b) : r_{ r }, g_{ g }, b_{ b } {}
	uint32_t GetCode() const;

private:
	uint8_t r_;
	uint8_t g_;
	uint8_t b_;
};

constexpr Color WHITE = { 255, 255, 255 };
constexpr Color LIGHT_GRAY = Color(192, 192, 192);
constexpr Color GRAY = Color(128, 128, 128);
constexpr Color DARK_GRAY = Color(64, 64, 64);
constexpr Color BLACK = Color(0, 0, 0);
constexpr Color RED = Color(255, 0, 0);
constexpr Color GREEN = Color(0, 255, 0);
constexpr Color BLUE = Color(0, 0, 255);
