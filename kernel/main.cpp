#include <cstdint>

#include "../UchLoaderPkg/frame_buffer_conf.hpp"
#include "../UchLoaderPkg/memory_map.hpp"
#include "graphics/font.hpp"
#include "graphics/kernel_logger.hpp"
#include "graphics/screen.hpp"
#include "interrupt/idt.hpp"
#include "memory/bootstrap_allocator.hpp"
#include "memory/buddy_system.hpp"
#include "memory/page.hpp"
#include "memory/paging.hpp"
#include "memory/segment.hpp"
#include "system_event_queue.hpp"
#include "task/task_manager.hpp"
#include "timers/acpi.hpp"
#include "timers/local_apic.hpp"
#include "timers/timer.hpp"

// 1MiB　
char kernel_stack[1024 * 1024];

extern "C" void Main(const FrameBufferConf& frame_buffer_conf,
					 const MemoryMap& memory_map,
					 const acpi::RSDP& rsdp)
{
	InitializeScreen(frame_buffer_conf, { 0, 120, 215 }, { 0, 80, 155 });

	InitializeFont();

	initialize_kernel_logger();

	InitializeSegmentation();

	InitializePaging();

	InitializeInterrupt();

	initialize_bootstrap_allocator(memory_map);

	initialize_heap();

	initialize_pages();

	initialize_memory_manager();

	disable_bootstrap_allocator();

	klogger->print("Hello, uch OS!\n");

	print_available_memory();

	acpi::Initialize(rsdp);

	local_apic::Initialize();

	InitializeTimer();

	InitializeTaskManager();

	HandleSystemEvents();
}