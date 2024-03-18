#include "elf.hpp"
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

	t->message_handlers[NOTIFY_TIMER_TIMEOUT] = +[](const message& m) {
		switch (m.data.timer.action) {
			case timeout_action_t::TERMINAL_CURSOR_BLINK: {
				// TODO: implement cursor blink for userland
				const message send_m = { NOTIFY_CURSOR_BLINK, 0, {} };
				send_message(SHELL_TASK_ID, &send_m);
				break;
			}
		}
	};

	t->message_handlers[IPC_FILE_SYSTEM_OPERATION] = +[](const message& m) {
		if (m.data.fs_operation.operation == FS_OP_LIST) {
			message send_m;
			send_m.type = NOTIFY_WRITE;
			send_m.data.write_shell.is_end_of_message = true;

			char path[30];
			memcpy(path, m.data.fs_operation.path, 30);

			auto* entry = file_system::find_directory_entry_by_path(path);
			if (entry == nullptr) {
				memcpy(send_m.data.write_shell.buf, "ls: No such file or directory",
					   30);
				send_message(SHELL_TASK_ID, &send_m);
				return;
			}

			if (entry->attribute == file_system::entry_attribute::ARCHIVE) {
				memcpy(send_m.data.write_shell.buf, path, strlen(path));
				send_message(SHELL_TASK_ID, &send_m);
				return;
			}

			int i = 0;
			char buf[128];
			auto entries = file_system::list_entries_in_directory(entry);
			for (const auto* e : entries) {
				char name[13];
				file_system::read_dir_entry_name(*e, name);
				to_lower(name);

				size_t len = strlen(name);

				memcpy(buf + i, name, len);
				buf[i + len] = '\n';
				i += len + 1;
			}

			memcpy(send_m.data.write_shell.buf, buf, 128);

			send_message(SHELL_TASK_ID, &send_m);
		} else if (m.data.fs_operation.operation == FS_OP_READ) {
			message send_m;
			send_m.type = NOTIFY_WRITE;
			send_m.data.write_shell.is_end_of_message = true;

			char path[30];
			memcpy(path, m.data.fs_operation.path, 30);

			auto* entry = file_system::find_directory_entry_by_path(path);
			if (entry == nullptr) {
				memcpy(send_m.data.write_shell.buf, "cat: No such file or directory",
					   30);
				send_message(SHELL_TASK_ID, &send_m);
				return;
			}

			file_system::load_file(send_m.data.write_shell.buf, entry->file_size,
								   *entry);
			send_m.data.write_shell.buf[entry->file_size] = '\0';

			send_message(SHELL_TASK_ID, &send_m);
		}
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

	std::vector<uint8_t> buf(entry->file_size);

	file_system::load_file(buf.data(), entry->file_size, *entry);

	exec_elf(buf.data(), "shell", nullptr);
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

	initialize_task();
	task* shell_task =
			create_task("shell", reinterpret_cast<uint64_t>(&task_shell), 2, true);
	schedule_task(shell_task->id);

	file_system::initialize_fat(volume_image);

	initialize_freetype();

	initialize_pci();

	usb::xhci::initialize();

	initialize_keyboard();

	task_main();
}
