#include <cstdint>

#include "../UchLoaderPkg/frame_buffer_conf.hpp"
#include "../UchLoaderPkg/memory_map.hpp"
#include "color.hpp"
#include "font.hpp"
#include "screen.hpp"

extern "C" void KernelMain(const FrameBufferConf& frame_buffer_conf,
                           const MemoryMap& memory_map) {
    Screen screen{frame_buffer_conf};

    Color background_color{0, 120, 215};
    Color taskbar_color{0, 80, 155};

    screen.FillRectangle(Point2D{0, 0}, {screen.Width(), screen.Height()},
                         background_color.GetCode());

    int taskbar_height = screen.Height() * 0.08;
    screen.FillRectangle({0, screen.Height() - taskbar_height},
                         {screen.Width(), taskbar_height},
                         taskbar_color.GetCode());

    BitmapFont font{8, 16};

    Color font_color{255, 255, 255};

    screen.DrawString({250, 100}, "uchos", font, font_color.GetCode());

    while (true) __asm__("hlt");
}