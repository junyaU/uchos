#include "task/builtin.hpp"
#include "file_system/fat.hpp"
#include "file_system/path.hpp"
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
#include <libs/common/process_id.hpp>

namespace
{
void notify_xhci_handler(const message& m) { usb::xhci::process_events(); }

void handle_initialize_task(const message& m)
{
	message send_m = { .type = msg_t::INITIALIZE_TASK, .sender = process_ids::KERNEL };
	send_m.data.init.task_id = m.sender.raw();
	send_message(m.sender, &send_m);
}

void handle_memory_usage(const message& m)
{
	message send_m = { .type = msg_t::IPC_MEMORY_USAGE, .sender = process_ids::KERNEL };

	size_t used_mem = 0;
	size_t total_mem = 0;

	get_memory_usage(&total_mem, &used_mem);

	send_m.data.memory_usage.total = total_mem;
	send_m.data.memory_usage.used = used_mem;

	send_message(m.sender, &send_m);
}

void handle_pci(const message& m)
{
	message send_m = { .type = msg_t::IPC_PCI,
					   .sender = process_ids::KERNEL,
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

void handle_fs_register_path(const message& m)
{
	path* p = reinterpret_cast<path*>(m.data.fs_op.buf);
	if (p == nullptr) {
		LOG_ERROR("failed to allocate memory");
		return;
	}

	task* t = get_task(m.sender);
	if (t->parent_id.raw() != -1) {
		auto* parent = get_task(t->parent_id);
		memcpy(&t->fs_path, &parent->fs_path, sizeof(path));
	} else {
		memcpy(&t->fs_path, p, sizeof(path));
	}

	kfree(p);

	message reply = { .type = msg_t::FS_REGISTER_PATH, .sender = process_ids::KERNEL };
	reply.data.fs_op.result = 0;
	send_message(m.sender, &reply);
};
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

	t->add_msg_handler(msg_t::INITIALIZE_TASK, handle_initialize_task);
	t->add_msg_handler(msg_t::IPC_MEMORY_USAGE, handle_memory_usage);
	t->add_msg_handler(msg_t::IPC_PCI, handle_pci);
	t->add_msg_handler(msg_t::FS_REGISTER_PATH, handle_fs_register_path);

	process_messages(t);
}

void task_shell()
{
	message m = { .type = msg_t::IPC_GET_FILE_INFO, .sender = process_ids::SHELL };
	char path[6] = "shell";
	memcpy(m.data.fs_op.name, path, 6);
	send_message(process_ids::FS_FAT32, &m);

	message info_m = wait_for_message(msg_t::IPC_GET_FILE_INFO);
	auto* entry =
			reinterpret_cast<file_system::directory_entry*>(info_m.data.fs_op.buf);
	if (entry == nullptr) {
		LOG_ERROR("failed to find shell");
		while (true) {
			__asm__("hlt");
		}
	}

	message read_m = { .type = msg_t::IPC_READ_FILE_DATA, .sender = process_ids::SHELL };
	read_m.data.fs_op.buf = info_m.data.fs_op.buf;
	send_message(process_ids::FS_FAT32, &read_m);

	message data_m = wait_for_message(msg_t::IPC_READ_FILE_DATA);
	CURRENT_TASK->is_initilized = true;
	file_system::execute_file(data_m.data.fs_op.buf, "shell", nullptr);
}

void task_usb_handler()
{
	task* t = CURRENT_TASK;

	initialize_pci();

	usb::xhci::initialize();

	initialize_keyboard();

	t->add_msg_handler(msg_t::NOTIFY_XHCI, notify_xhci_handler);

	process_messages(t);
}
