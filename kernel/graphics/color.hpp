#pragma once

#include <cstdint>

namespace
{
const int kBytesPerPixel = 4;
}

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