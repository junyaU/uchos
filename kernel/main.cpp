#include "file_system/fat.hpp"
#include "graphics/font.hpp"
#include "graphics/screen.hpp"
#include "hardware/keyboard.hpp"
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
#include "task/builtin.hpp"
#include "task/task.hpp"
#include "timers/acpi.hpp"
#include "timers/local_apic.hpp"
#include "timers/timer.hpp"
#include <libs/common/types.hpp>

struct FrameBufferConf;
struct MemoryMap;

// 1MiB　
char kernel_stack[1024 * 1024];

extern "C" void Main(const FrameBufferConf& frame_buffer_conf,
					 const MemoryMap& memory_map,
					 const acpi::root_system_description_pointer& rsdp,
					 void* volume_image)
{
	initialize_screen(frame_buffer_conf, { 0, 0, 0 });

	initialize_font();

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

	acpi::initialize(rsdp);

	initialize_timer();

	local_apic::initialize();

	syscall::initialize();

	file_system::initialize_fat(volume_image);

	initialize_task();

	initialize_freetype();

	initialize_pci();

	usb::xhci::initialize();

	initialize_keyboard();

	task_kernel();
}
