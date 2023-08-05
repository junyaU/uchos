#pragma once

#include <array>
#include <cstdint>

#include "color.hpp"

class SystemLogger {
   public:
    static const int kLines = 25, kCharsPerLine = 80;
    SystemLogger(Color font_color);

    void Print(const char* s);
    void Clear();

   private:
    void NextLine();
    void ScrollLines();

    char buffer_[kLines][kCharsPerLine + 1];
    int cursor_y_{0};
    int cursor_x_{0};
    Color font_color_;
};

extern SystemLogger* system_logger;

void InitializeSystemLogger();