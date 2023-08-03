#include "font.hpp"

#include <cstdint>

extern const uint8_t _binary_hankaku_bin_start;
extern const uint8_t _binary_hankaku_bin_end;
extern const uint8_t _binary_hankaku_bin_size;

BitmapFont::BitmapFont(int width, int height)
    : font_data_{&_binary_hankaku_bin_start}, width_{width}, height_{height} {}

const uint8_t* BitmapFont::GetFont(char c) {
    auto index = height_ * static_cast<unsigned int>(c);
    if (index >= reinterpret_cast<uintptr_t>(&_binary_hankaku_bin_size)) {
        return nullptr;
    }

    return &font_data_[index];
}