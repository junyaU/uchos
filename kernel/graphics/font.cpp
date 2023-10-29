#include "font.hpp"

#include <new>

extern const uint8_t _binary_hankaku_bin_start;
extern const uint8_t _binary_hankaku_bin_end;
extern const uint8_t _binary_hankaku_bin_size;

bitmap_font::bitmap_font(int width, int height)
	: font_data_{ &_binary_hankaku_bin_start }, width_{ width }, height_{ height }
{
}

const uint8_t* bitmap_font::get_font(char c)
{
	auto index = height_ * static_cast<unsigned int>(c);
	if (index >= reinterpret_cast<uintptr_t>(&_binary_hankaku_bin_size)) {
		return nullptr;
	}

	return &font_data_[index];
}

bitmap_font* kfont;
alignas(bitmap_font) char bitmap_font_buffer[sizeof(bitmap_font)];

void initialize_font() { kfont = new (bitmap_font_buffer) bitmap_font{ 8, 16 }; }