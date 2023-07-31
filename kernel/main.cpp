#include <cstdint>

#include "../UchLoaderPkg/frame_buffer_conf.hpp"

extern "C" void KernelMain(const FrameBufferConf& frame_buffer_conf) {
    for (uint64_t y = 0; y < frame_buffer_conf.vertical_resolution; y++) {
        for (uint64_t x = 0; x < frame_buffer_conf.horizontal_resolution; x++) {
            const uint64_t i = y * frame_buffer_conf.pixels_per_scan_line + x;
            frame_buffer_conf.frame_buffer[i * 4 + 0] = 255;
            frame_buffer_conf.frame_buffer[i * 4 + 1] = 255;
            frame_buffer_conf.frame_buffer[i * 4 + 2] = 255;
        }
    }

    while (true) __asm__("hlt");
}