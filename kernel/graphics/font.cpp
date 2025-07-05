#include "font.hpp"
#include "log.hpp"
#include "point2d.hpp"
#include "screen.hpp"
#include <cctype>
#include <cstdint>
#include <vector>

extern const uint8_t _binary_hankaku_bin_start;
extern const uint8_t _binary_hankaku_bin_end;
extern const uint8_t _binary_hankaku_bin_size;

FT_Library ft_library;
std::vector<uint8_t>* nihongo_font_data;

namespace kernel::graphics {

bitmap_font::bitmap_font(int width, int height)
	: font_data_{ &_binary_hankaku_bin_start }, width_{ width }, height_{ height }
{
}

bool is_ascii_code(char32_t c) { return c <= 0x7E; }

const uint8_t* bitmap_font::get_font(char c)
{
	auto index = height_ * static_cast<unsigned int>(c);
	if (index >= reinterpret_cast<uintptr_t>(&_binary_hankaku_bin_size)) {
		return nullptr;
	}

	return &font_data_[index];
}

void render_unicode(char32_t c, FT_Face face)
{
	const auto glyph_index = FT_Get_Char_Index(face, c);
	if (glyph_index == 0) {
		LOG_ERROR("Glyph not found");
		return;
	}

	if (const int err = FT_Load_Glyph(face, glyph_index,
								FT_LOAD_RENDER | FT_LOAD_TARGET_MONO)) {
		LOG_ERROR("Failed to load glyph: %d", err);
		return;
	}
}

void write_ascii(screen& scr, point2d position, char c, uint32_t color_code)
{
	const uint8_t* font = kfont->get_font(c);
	if (font != nullptr) {
		for (int dy = 0; dy < kfont->height(); dy++) {
			for (int dx = 0; dx < kfont->width(); dx++) {
				if ((font[dy] << dx & 0x80) != 0) {
					scr.put_pixel(position + point2d{ dx, dy }, color_code);
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

void write_unicode(screen& scr, point2d position, char32_t c, uint32_t color_code)
{
	{
		if (is_ascii_code(c)) {
			write_ascii(scr, position, c, color_code);
			return;
		}

		auto face = nullptr; // new_face();
		if (face == nullptr) {
			write_ascii(scr, position, '?', color_code);
			write_ascii(scr, position + point2d{ kfont->width(), 0 }, '?',
						color_code);
			return;
		}

		// render_unicode(c, face);

		// FT_Bitmap& bitmap = face->glyph->bitmap;
		// const int baseline = (face->height + face->descender) *
		// 					 face->size->metrics.y_ppem / face->units_per_EM;
		// const auto glyph_topleft =
		// 		position + point2d{ face->glyph->bitmap_left,
		// 							baseline - face->glyph->bitmap_top };

		// for (int dy = 0; dy < bitmap.rows; ++dy) {
		// 	unsigned char* q = &bitmap.buffer[bitmap.pitch * dy];
		// 	if (bitmap.pitch < 0) {
		// 		q -= bitmap.pitch * bitmap.rows;
		// 	}
		// 	for (int dx = 0; dx < bitmap.width; ++dx) {
		// 		const bool b = (q[dx >> 3] & (0x80 >> (dx & 0x7))) != 0;
		// 		if (b) {
		// 			kscreen->put_pixel(glyph_topleft + point2d{ dx, dy },
		// 							   color_code);
		// 		}
		// 	}
		// }

		// FT_Done_Face(face);
	}
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

char decode_utf8(char32_t c)
{
	if (c <= 0x7F) {
		return static_cast<char>(c);
	}

	if (c <= 0x7FF) {
		return static_cast<char>(0xC0 | (c >> 6));
	}

	if (c <= 0xFFFF) {
		return static_cast<char>(0xE0 | (c >> 12));
	}

	if (c <= 0x10FFFF) {
		return static_cast<char>(0xF0 | (c >> 18));
	}

	return 0;
}

void write_string(screen& scr, point2d position, const char* s, uint32_t color_code)
{
	int font_position = 0;
	while (*s != '\0') {
		const int size = utf8_size(*s);
		const char32_t c = utf8_to_unicode(s);

		write_unicode(scr, position + point2d{ font_position * kfont->width(), 0 },
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

bitmap_font* kfont;
alignas(bitmap_font) char bitmap_font_buffer[sizeof(bitmap_font)];

FT_Face new_face()
{
	FT_Face face;

	if (const int err = FT_New_Memory_Face(ft_library, nihongo_font_data->data(),
									 nihongo_font_data->size(), 0, &face)) {
		LOG_ERROR("Failed to create new face: %d", err);
		return 0;
	}

	if (const int err = FT_Set_Pixel_Sizes(face, 16, 16)) {
		LOG_ERROR("Failed to set pixel size: %d", err);
		return 0;
	}

	return face;
}

void initialize_font() { kfont = new (bitmap_font_buffer) bitmap_font{ 8, 16 }; }

void initialize_freetype()
{
	if (const int err = FT_Init_FreeType(&ft_library)) {
		LOG_ERROR("Failed to initialize FreeType: %d", err);
		return;
	}
}

} // namespace kernel::graphics