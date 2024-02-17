#include "graphics/color.hpp"

Color::Color(uint8_t r, uint8_t g, uint8_t b) : r_(r), g_(g), b_(b) {}

uint32_t Color::GetCode() const
{
	uint32_t code = 0;
	code += r_;
	code <<= 8;
	code += g_;
	code <<= 8;
	code += b_;
	return code;
}