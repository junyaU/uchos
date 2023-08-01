#include "screen.hpp"

Screen::Screen(const FrameBufferConf& frame_buffer_conf)
    : pixels_per_scan_line_(frame_buffer_conf.pixels_per_scan_line),
      horizontal_resolution_(frame_buffer_conf.horizontal_resolution),
      vertical_resolution_(frame_buffer_conf.vertical_resolution),
      frame_buffer_(
          reinterpret_cast<uint32_t*>(frame_buffer_conf.frame_buffer)) {}

void Screen::PutPixel(Point2D point, const uint32_t color_code) {
    const uint64_t pixel_position =
        pixels_per_scan_line_ * point.GetY() + point.GetX();
    frame_buffer_[pixel_position] = color_code;
}

void Screen::FillRectangle(Point2D position, Point2D size,
                           const uint32_t color_code) {
    for (int dy = 0; dy < size.GetY(); dy++) {
        for (int dx = 0; dx < size.GetX(); dx++) {
            PutPixel(position + Point2D{dx, dy}, color_code);
        }
    }
}
