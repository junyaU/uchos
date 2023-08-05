#include "system_logger.hpp"

#include "font.hpp"
#include "screen.hpp"

SystemLogger::SystemLogger(Color font_color) : font_color_{font_color} {}

void SystemLogger::Print(const char* s) {
    while (*s) {
        if (*s == '\n') {
            NextLine();
            s++;
            continue;
        }

        buffer_[cursor_y_][cursor_x_] = *s;
        screen->DrawString(Point2D{cursor_x_ * bitmap_font->Width(),
                                   cursor_y_ * bitmap_font->Height()},
                           *s, font_color_.GetCode());

        if (cursor_x_ == kCharsPerLine) {
            NextLine();
        } else {
            cursor_x_++;
        }

        s++;
    }
}

void SystemLogger::NextLine() {
    cursor_x_ = 0;
    cursor_y_++;
}

SystemLogger* system_logger;
char system_logger_buf[sizeof(SystemLogger)];

void InitializeSystemLogger() {
    system_logger = new (system_logger_buf) SystemLogger{Color{255, 255, 255}};
}
