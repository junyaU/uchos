#include <cstdint>

#include "../UchLoaderPkg/frame_buffer_conf.hpp"
#include "../UchLoaderPkg/memory_map.hpp"
#include "graphics/font.hpp"
#include "graphics/screen.hpp"
#include "graphics/system_logger.hpp"
#include "interrupt/idt.hpp"
#include "memory/buddy_system.hpp"
#include "memory/paging.hpp"
#include "memory/segment.hpp"
#include "timers/acpi.hpp"
#include "timers/local_apic.hpp"
#include "timers/timer.hpp"

#include "system_event_queue.hpp"

// 1MiB
char kernel_stack[1024 * 1024];

extern "C" void Main(const FrameBufferConf& frame_buffer_conf,
					 const MemoryMap& memory_map,
					 const acpi::RSDP& rsdp)
{
	InitializeScreen(frame_buffer_conf, { 0, 120, 215 }, { 0, 80, 155 });

	InitializeFont();

	InitializeSystemLogger();

	InitializeSegmentation();

	InitializePaging();

	InitializeInterrupt();

	InitializeBuddySystem(memory_map);

	InitializeHeap();

	system_logger->Print("Hello, uch OS!\n");

	acpi::Initialize(rsdp);

	local_apic::Initialize();

	InitializeTimer();

	timer->AddTimerEvent(2500, true);

	HandleSystemEvents();
}