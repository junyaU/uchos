#include "font.hpp"
#include <cctype>
#include <cstdint>
#include <new> // NOLINT(misc-include-cleaner)
#include "log.hpp"
#include "point2d.hpp"
#include "screen.hpp"

extern const uint8_t _binary_hankaku_bin_start;
extern const uint8_t _binary_hankaku_bin_end;
extern const uint8_t _binary_hankaku_bin_size;

namespace kernel::graphics
{

BitmapFont::BitmapFont(int width, int height)
	: font_data_{ &_binary_hankaku_bin_start }, width_{ width }, height_{ height }
{
}

bool is_ascii_code(char32_t c) { return c <= 0x7E; }

const uint8_t* BitmapFont::get_font(char c)
{
	auto index = height_ * static_cast<unsigned int>(c);
	if (index >= reinterpret_cast<uintptr_t>(&_binary_hankaku_bin_size)) {
		return nullptr;
	}

	return &font_data_[index];
}

void write_ascii(Screen& scr, Point2D position, char c, uint32_t color_code)
{
	const uint8_t* font = kfont->get_font(c);
	if (font != nullptr) {
		for (int dy = 0; dy < kfont->height(); dy++) {
			for (int dx = 0; dx < kfont->width(); dx++) {
				if ((font[dy] << dx & 0x80) != 0) {
					scr.put_pixel(position + Point2D{ dx, dy }, color_code);
				}
			}
		}
	}
}

int utf8_size(uint8_t c)
{
	if (c < 0x80) {
		return 1;
	}

	if (0xc0 <= c && c < 0xe0) {
		return 2;
	}

	if (0xe0 <= c && c < 0xf0) {
		return 3;
	}

	if (0xf0 <= c && c < 0xf8) {
		return 4;
	}

	return 0;
}

void write_unicode(Screen& scr, Point2D position, char32_t c, uint32_t color_code)
{
	if (is_ascii_code(c)) {
		write_ascii(scr, position, c, color_code);
		return;
	}

	// Non-ASCII glyph rendering (e.g. via FreeType) is not implemented yet;
	// fall back to a placeholder.
	write_ascii(scr, position, '?', color_code);
	write_ascii(scr, position + Point2D{ kfont->width(), 0 }, '?', color_code);
}

char32_t utf8_to_unicode(const char* utf8)
{
	switch (utf8_size(*utf8)) {
		case 1:
			return static_cast<char32_t>(utf8[0]);
		case 2:
			return (static_cast<char32_t>(utf8[0]) & 0b0001'1111) << 6 |
				   (static_cast<char32_t>(utf8[1]) & 0b0011'1111) << 0;
		case 3:
			return (static_cast<char32_t>(utf8[0]) & 0b0000'1111) << 12 |
				   (static_cast<char32_t>(utf8[1]) & 0b0011'1111) << 6 |
				   (static_cast<char32_t>(utf8[2]) & 0b0011'1111) << 0;
		case 4:
			return (static_cast<char32_t>(utf8[0]) & 0b0000'0111) << 18 |
				   (static_cast<char32_t>(utf8[1]) & 0b0011'1111) << 12 |
				   (static_cast<char32_t>(utf8[2]) & 0b0011'1111) << 6 |
				   (static_cast<char32_t>(utf8[3]) & 0b0011'1111) << 0;
		default:
			return 0;
	}
}

void write_string(Screen& scr, Point2D position, const char* s, uint32_t color_code)
{
	int font_position = 0;
	while (*s != '\0') {
		const int size = utf8_size(*s);
		const char32_t c = utf8_to_unicode(s);

		write_unicode(scr, position + Point2D{ font_position * kfont->width(), 0 },
					  c, color_code);

		font_position += is_ascii_code(c) ? 1 : 2;
		s += size;
	}
}

void to_lower(char* s)
{
	if (s == nullptr) {
		return;
	}

	while (*s != '\0') {
		*s = std::tolower(*s);
		++s;
	}
}

void to_upper(char* s)
{
	if (s == nullptr) {
		return;
	}

	while (*s != '\0') {
		*s = std::toupper(static_cast<unsigned char>(*s));
		++s;
	}
}

BitmapFont* kfont;
alignas(BitmapFont) char bitmap_font_buffer[sizeof(BitmapFont)];

void initialize_font() { kfont = new (bitmap_font_buffer) BitmapFont{ 8, 16 }; }

} // namespace kernel::graphics
