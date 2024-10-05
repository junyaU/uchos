#include "task/builtin.hpp"
#include "file_system/fat.hpp"
#include "graphics/log.hpp"
#include "hardware/keyboard.hpp"
#include "hardware/pci.hpp"
#include "hardware/usb/xhci/xhci.hpp"
#include "hardware/virtio/pci.hpp"
#include "memory/page.hpp"
#include "task/ipc.hpp"
#include "task/task.hpp"
#include <cstddef>
#include <cstring>
#include <libs/common/message.hpp>
#include <libs/common/types.hpp>

namespace
{
void notify_xhci_handler(const message& m) { usb::xhci::process_events(); }

void handle_initialize_task(const message& m)
{
	message send_m = { .type = IPC_INITIALIZE_TASK, .sender = KERNEL_TASK_ID };
	send_m.data.init.task_id = m.sender;
	send_message(SHELL_TASK_ID, &send_m);
}

void handle_memory_usage(const message& m)
{
	message send_m = { .type = IPC_MEMORY_USAGE, .sender = KERNEL_TASK_ID };

	size_t used_mem = 0;
	size_t total_mem = 0;

	get_memory_usage(&total_mem, &used_mem);

	send_m.data.memory_usage.total = total_mem;
	send_m.data.memory_usage.used = used_mem;

	send_message(m.sender, &send_m);
}

void handle_pci(const message& m)
{
	message send_m = { .type = IPC_PCI,
					   .sender = KERNEL_TASK_ID,
					   .is_end_of_message = false };

	for (size_t i = 0; i < pci::num_devices; ++i) {
		auto& device = pci::devices[i];
		send_m.data.pci.vendor_id = device.vendor_id;
		send_m.data.pci.device_id = device.device_id;
		device.address(send_m.data.pci.bus_address, 8);

		if (i == pci::num_devices - 1) {
			send_m.is_end_of_message = true;
		}

		send_message(m.sender, &send_m);
	}
}
} // namespace

void task_idle()
{
	while (true) {
		__asm__("hlt");
	}
}

void task_kernel()
{
	task* t = CURRENT_TASK;

	t->message_handlers[IPC_INITIALIZE_TASK] = handle_initialize_task;
	t->message_handlers[IPC_MEMORY_USAGE] = handle_memory_usage;
	t->message_handlers[IPC_PCI] = handle_pci;

	process_messages(t);
}

void task_shell()
{
	task* t = CURRENT_TASK;

	message m = { .type = IPC_GET_FILE_INFO, .sender = SHELL_TASK_ID };
	char path[6] = "shell";
	memcpy(m.data.fs_op.path, path, 6);
	send_message(FS_FAT32_TASK_ID, &m);

	message info_m = wait_for_message(IPC_GET_FILE_INFO);
	auto* entry =
			reinterpret_cast<file_system::directory_entry*>(info_m.data.fs_op.buf);
	if (entry == nullptr) {
		LOG_ERROR("failed to find shell");
		while (true) {
			__asm__("hlt");
		}
	}

	message read_m = { .type = IPC_READ_FILE_DATA, .sender = SHELL_TASK_ID };
	read_m.data.fs_op.buf = info_m.data.fs_op.buf;
	send_message(FS_FAT32_TASK_ID, &read_m);

	message data_m = wait_for_message(IPC_READ_FILE_DATA);
	CURRENT_TASK->is_initilized = true;
	file_system::execute_file(data_m.data.fs_op.buf, "shell", nullptr);
}

void task_usb_handler()
{
	task* t = CURRENT_TASK;

	initialize_pci();

	usb::xhci::initialize();

	initialize_keyboard();

	t->message_handlers[NOTIFY_XHCI] = notify_xhci_handler;

	process_messages(t);
}
