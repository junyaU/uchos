struct FrameBufferConf;
struct MemoryMap;

#include "graphics/font.hpp"
#include "graphics/kernel_logger.hpp"
#include "graphics/screen.hpp"
#include "hardware/pci.hpp"
#include "hardware/usb/xhci/xhci.hpp"
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
	initialize_screen(frame_buffer_conf, { 0, 0, 0 });

	initialize_font();

	initialize_kernel_logger();

	initialize_segmentation();

	initialize_paging();

	initialize_interrupt();

	initialize_bootstrap_allocator(memory_map);

	initialize_heap();

	initialize_pages();

	initialize_memory_manager();

	disable_bootstrap_allocator();

	initialize_slab_allocator();

	print_available_memory();

	acpi::initialize(rsdp);

	local_apic::initialize();

	initialize_timer();

	initialize_task_manager();

	klogger->print("Hello, uchos\n");

	initialize_pci();

	usb::xhci::initialize();

	usb::xhci::process_events();

	handle_system_events();
}