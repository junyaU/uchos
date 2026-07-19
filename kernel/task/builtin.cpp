#include "task/builtin.hpp"
#include <cstddef>
#include <cstring>
#include <libs/common/message.hpp>
#include <libs/common/process_id.hpp>
#include <libs/common/types.hpp>
#include "fs/fat/fat.hpp"
#include "fs/path.hpp"
#include "hardware/keyboard.hpp"
#include "hardware/pci.hpp"
#include "hardware/usb/xhci/xhci.hpp"
#include "hardware/virtio/pci.hpp"
#include "log/log.hpp"
#include "memory/page.hpp"
#include "memory/slab.hpp"
#include "task/ipc.hpp"
#include "task/task.hpp"

namespace
{
void notify_xhci_handler(const Message& m)
{
	kernel::hw::usb::xhci::process_events();
}

void handle_initialize_task(const Message& m)
{
	Message send_m = { .type = MsgType::INITIALIZE_TASK,
					   .sender = process_ids::KERNEL };
	send_m.data.init.task_id = m.sender.raw();
	kernel::task::send_message(m.sender, send_m);
}

void handle_memory_usage(const Message& m)
{
	Message resp = { .type = MsgType::IPC_MEMORY_USAGE,
					 .sender = process_ids::KERNEL };

	size_t used_mem = 0;
	size_t total_mem = 0;

	kernel::memory::get_memory_usage(&total_mem, &used_mem);

	resp.data.memory_usage.total = total_mem;
	resp.data.memory_usage.used = used_mem;
	resp.result = OK;

	kernel::task::reply(m, &resp);
}

void handle_pci(const Message& m)
{
	Message send_m = {
		.type = MsgType::IPC_PCI,
		.sender = process_ids::KERNEL,
	};

	for (size_t i = 0; i < kernel::hw::pci::num_devices; ++i) {
		auto& device = kernel::hw::pci::devices[i];
		send_m.data.pci.vendor_id = device.vendor_id;
		send_m.data.pci.device_id = device.device_id;
		device.address(send_m.data.pci.bus_address, 8);

		// if (i == kernel::hw::pci::num_devices - 1) {
		// }

		kernel::task::send_message(m.sender, send_m);
	}
}

} // namespace

namespace kernel::task
{

void idle_service()
{
	while (true) {
		__asm__("hlt");
	}
}

void kernel_service()
{
	Task* t = kernel::task::CURRENT_TASK;

	t->add_msg_handler(MsgType::INITIALIZE_TASK, handle_initialize_task);
	t->add_msg_handler(MsgType::IPC_MEMORY_USAGE, handle_memory_usage);
	t->add_msg_handler(MsgType::IPC_PCI, handle_pci);

	kernel::task::process_messages(t);
}

void shell_service()
{
	Message m = { .type = MsgType::IPC_GET_FILE_INFO, .sender = process_ids::SHELL };
	char path[6] = "shell";
	memcpy(m.data.fs.name, path, 6);

	const error_t info_err = kernel::task::call(process_ids::FS_FAT32, &m);
	auto* entry = reinterpret_cast<kernel::fs::DirectoryEntry*>(m.data.fs.buf);
	if (IS_ERR(info_err) || IS_ERR(m.result) || entry == nullptr) {
		LOG_ERROR("failed to find shell");
		while (true) {
			__asm__("hlt");
		}
	}

	Message read_m = { .type = MsgType::IPC_READ_FILE_DATA,
					   .sender = process_ids::SHELL };
	read_m.data.fs.buf = entry;
	const error_t read_err = kernel::task::call(process_ids::FS_FAT32, &read_m);
	kernel::memory::free(entry);
	if (IS_ERR(read_err) || IS_ERR(read_m.result) || read_m.data.fs.buf == nullptr) {
		LOG_ERROR("failed to read shell binary");
		while (true) {
			__asm__("hlt");
		}
	}

	CURRENT_TASK->is_initialized = true;
	kernel::fs::fat::execute_file(read_m.data.fs.buf, "shell", nullptr);
}

void usb_handler_service()
{
	Task* t = kernel::task::CURRENT_TASK;

	kernel::hw::pci::initialize();

	kernel::hw::usb::xhci::initialize();

	kernel::hw::initialize_keyboard();

	t->add_msg_handler(MsgType::NOTIFY_XHCI, notify_xhci_handler);

	kernel::task::process_messages(t);
}

} // namespace kernel::task
