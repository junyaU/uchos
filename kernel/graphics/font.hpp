#pragma once

#include <cstdint>

#include <ft2build.h>
#include FT_FREETYPE_H

#include "../point2d.hpp"

namespace kernel::graphics {

class bitmap_font
{
public:
	bitmap_font(int width, int height);
	const uint8_t* get_font(char c);
	int width() const { return width_; }
	int height() const { return height_; }
	point2d size() const { return { width_, height_ }; }

private:
	const uint8_t* font_data_;
	int width_;
	int height_;
};

class screen;

bool is_ascii_code(char32_t c);

int utf8_size(uint8_t c);

char32_t utf8_to_unicode(const char* utf8);

char decode_utf8(char32_t c);

void write_ascii(screen& scr, point2d position, char c, uint32_t color_code);

void write_unicode(screen& scr, point2d position, char32_t c, uint32_t color_code);

void write_string(screen& scr, point2d position, const char* s, uint32_t color_code);

void to_lower(char* s);

void to_upper(char* s);

extern bitmap_font* kfont;

} // namespace kernel::graphics

FT_Face new_face();

void initialize_font();

void initialize_freetype();