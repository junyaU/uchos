#include <cstdint>

#include "../UchLoaderPkg/frame_buffer_conf.hpp"
#include "../UchLoaderPkg/memory_map.hpp"
#include "color.hpp"
#include "font.hpp"
#include "screen.hpp"
#include "system_logger.hpp"

extern "C" void KernelMain(const FrameBufferConf& frame_buffer_conf,
                           const MemoryMap& memory_map) {
    InitializeScreen(frame_buffer_conf, {0, 120, 215}, {0, 80, 155});

    InitializeFont();

    InitializeSystemLogger();

    system_logger->Print("Hello, uchan OS!\n");

    while (true) __asm__("hlt");
}