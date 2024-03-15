#pragma once

#include <cstdint>

constexpr int FONT_WIDTH = 8;
constexpr int FONT_HEIGHT = 16;

void print_text(int x, int y, const char* text, uint32_t color);

void delete_char(int x, int y);

void clear_screen();
