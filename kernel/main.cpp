#include <cstdint>

#include "../UchLoaderPkg/frame_buffer_conf.hpp"
#include "../UchLoaderPkg/memory_map.hpp"
#include "color.hpp"
#include "font.hpp"
#include "screen.hpp"

extern "C" void KernelMain(const FrameBufferConf& frame_buffer_conf,
                           const MemoryMap& memory_map) {
    InitializeScreen(frame_buffer_conf);

    InitializeFont();

    Color font_color{255, 255, 255};
    screen->DrawString({250, 100}, "uchos", font_color.GetCode());

    while (true) __asm__("hlt");
}