struct FrameBufferConf;
struct MemoryMap;

#include "graphics/font.hpp"
#include "graphics/kernel_logger.hpp"
#include "graphics/screen.hpp"
#include "interrupt/idt.hpp"
#include "memory/bootstrap_allocator.hpp"
#include "memory/buddy_system.hpp"
#include "memory/page.hpp"
#include "memory/paging.hpp"
#include "memory/segment.hpp"
#include "memory/slab.hpp"
#include "system_event_queue.hpp"
#include "task/task_manager.hpp"
#include "timers/acpi.hpp"
#include "timers/local_apic.hpp"
#include "timers/timer.hpp"

// 1MiB　
char kernel_stack[1024 * 1024];

extern "C" void Main(const FrameBufferConf& frame_buffer_conf,
					 const MemoryMap& memory_map,
					 const acpi::root_system_description_pointer& rsdp)
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

	initialize_slab_allocator();

	acpi::initialize(rsdp);

	local_apic::Initialize();

	initialize_timer();

	klogger->print("Hello, uch OS!\n");

	InitializeTaskManager();

	print_available_memory();

	m_cache_create(nullptr, sizeof(kernel_timer));

	auto start = acpi::get_pm_timer_count();
	void* addr = kmalloc(sizeof(kernel_timer));
	auto end = acpi::get_pm_timer_count();

	klogger->printf("kmalloc: %f\n", acpi::pm_timer_count_to_millisec(end - start));

	if (addr == nullptr) {
		klogger->print("failed to allocate memory\n");
		return;
	}

	kfree(addr);

	HandleSystemEvents();
}