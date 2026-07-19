#pragma once

#include <cstdint>

constexpr int FONT_WIDTH = 8;
constexpr int FONT_HEIGHT = 16;

constexpr int SCREEN_WIDTH = 800;
constexpr int SCREEN_HEIGHT = 600;

constexpr uint32_t COLOR_BLACK = 0x000000;

void print_text(int x, int y, const char* text, uint32_t color);

void delete_char(int x, int y);

void clear_screen();
