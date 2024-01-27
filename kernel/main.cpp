struct FrameBufferConf;
struct MemoryMap;

#include "file_system/fat.hpp"
#include "graphics/font.hpp"
#include "graphics/screen.hpp"
#include "graphics/terminal.hpp"
#include "hardware/keyboad.hpp"
#include "hardware/pci.hpp"
#include "hardware/usb/xhci/xhci.hpp"
#include "interrupt/idt.hpp"
#include "memory/bootstrap_allocator.hpp"
#include "memory/buddy_system.hpp"
#include "memory/page.hpp"
#include "memory/paging.hpp"
#include "memory/segment.hpp"
#include "memory/slab.hpp"
#include "syscall/syscall.hpp"
#include "system_event_queue.hpp"
#include "task/task_manager.hpp"
#include "timers/acpi.hpp"
#include "timers/local_apic.hpp"
#include "timers/timer.hpp"

// 1MiB　
char kernel_stack[1024 * 1024];

extern "C" void Main(const FrameBufferConf& frame_buffer_conf,
					 const MemoryMap& memory_map,
					 const acpi::root_system_description_pointer& rsdp,
					 void* volume_image)
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

	initialize_tss();

	print_available_memory();

	initialize_system_event_queue();

	acpi::initialize(rsdp);

	initialize_timer();

	local_apic::initialize();

	syscall::initialize();

	initialize_task_manager();

	file_system::initialize_fat(volume_image);

	main_terminal->initialize_command_line();

	initialize_pci();

	usb::xhci::initialize();

	initialize_keyboard();

	handle_system_events();
}
