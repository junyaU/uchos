#pragma once

#include <cstdint>

#include <ft2build.h>
#include FT_FREETYPE_H

#include "../point2d.hpp"

class bitmap_font
{
public:
	bitmap_font(int width, int height);
	const uint8_t* get_font(char c);
	int width() const { return width_; }
	int height() const { return height_; }
	Point2D size() const { return { width_, height_ }; }

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

void write_ascii(screen& scr, Point2D position, char c, uint32_t color_code);

void write_unicode(screen& scr, Point2D position, char32_t c, uint32_t color_code);

void write_string(screen& scr, Point2D position, const char* s, uint32_t color_code);

extern bitmap_font* kfont;

FT_Face new_face();

void initialize_font();

void initialize_freetype();