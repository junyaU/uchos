#include "file_system/fat.hpp"
#include "graphics/font.hpp"
#include "graphics/log.hpp"
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
#include "task/ipc.hpp"
#include "task/task.hpp"
#include "timers/acpi.hpp"
#include "timers/local_apic.hpp"
#include "timers/timer.hpp"
#include "types.hpp"

struct FrameBufferConf;
struct MemoryMap;

void task_main()
{
	task* t = CURRENT_TASK;

	t->message_handlers[IPC_INITIALIZE_TASK] = +[](const message& m) {
		message send_m;
		send_m.type = IPC_INITIALIZE_TASK;
		send_m.data.init.task_id = m.sender;
		send_message(SHELL_TASK_ID, &send_m);
	};

	process_messages(t);
}

void task_shell()
{
	auto* entry = file_system::find_directory_entry_by_path("/shell");
	if (entry == nullptr) {
		printk(KERN_ERROR, "failed to find /shell");
		return;
	}

	file_system::execute_file(*entry, nullptr);
}

void task_sandbox()
{
	auto* file = file_system::find_directory_entry_by_path("/sandbox");
	if (file == nullptr) {
		printk(KERN_ERROR, "failed to find /sandbox");
		return;
	}

	file_system::execute_file(*file, nullptr);
}

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

	print_available_memory();

	acpi::initialize(rsdp);

	initialize_timer();

	local_apic::initialize();

	syscall::initialize();

	file_system::initialize_fat(volume_image);

	initialize_task();

	task* shell_task =
			create_task("shell", reinterpret_cast<uint64_t>(&task_shell), true);
	schedule_task(shell_task->id);

	initialize_freetype();

	initialize_pci();

	usb::xhci::initialize();

	initialize_keyboard();

	task_main();
}
