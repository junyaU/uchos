#include <cstdint>

#include "../UchLoaderPkg/frame_buffer_conf.hpp"
#include "../UchLoaderPkg/memory_map.hpp"
#include "graphics/font.hpp"
#include "graphics/screen.hpp"
#include "graphics/system_logger.hpp"
#include "interrupt/idt.hpp"
#include "timers/acpi.hpp"
#include "timers/local_apic.hpp"

extern "C" void KernelMain(const FrameBufferConf& frame_buffer_conf,
                           const MemoryMap& memory_map,
                           const acpi::RSDP& rsdp) {
    InitializeScreen(frame_buffer_conf, {0, 120, 215}, {0, 80, 155});

    InitializeFont();

    InitializeSystemLogger();

    InitializeInterrupt();

    system_logger->Print("Hello, uch OS!\n");

    acpi::Initialize(rsdp);

    local_apic::Initialize();

    asm volatile("int $0x40");

    while (true) __asm__("hlt");
}