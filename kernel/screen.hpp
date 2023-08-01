#pragma once

#include <cstdint>

#include "../UchLoaderPkg/frame_buffer_conf.hpp"
#include "color.hpp"
#include "point2d.hpp"

class Screen {
   public:
    Screen(const FrameBufferConf& frame_buffer_conf);
    const int Width() const { return horizontal_resolution_; }
    const int Height() const { return vertical_resolution_; }

    void PutPixel(Point2D point, const uint32_t color_code);
    void FillRectangle(Point2D position, Point2D size,
                       const uint32_t color_code);

   private:
    uint64_t pixels_per_scan_line_;
    uint64_t horizontal_resolution_;
    uint64_t vertical_resolution_;
    uint32_t* frame_buffer_;
};

void InitializeScreen(const FrameBufferConf& frame_buffer_conf);
