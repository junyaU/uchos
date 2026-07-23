#include "task/builtin.hpp"
#include <cstddef>
#include <cstring>
#include <libs/common/message.hpp>
#include <libs/common/process_id.hpp>
#include <libs/common/types.hpp>
#include <utility>
#include "elf.hpp"
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

void handle_task_ready(const Message& m)
{
	Message send_m = { .type = MsgType::KERNEL_TASK_READY,
					   .sender = process_ids::KERNEL };
	send_m.data.init.task_id = m.sender.raw();
	kernel::task::send_message(m.sender, send_m);
}

void handle_memory_usage(const Message& m)
{
	Message resp = { .type = MsgType::KERNEL_MEMORY_USAGE,
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
	Message resp = {
		.type = MsgType::KERNEL_PCI_LIST,
		.sender = process_ids::KERNEL,
	};

	// One reply with the whole device table as an OOL array replaces the
	// old one-request-N-responses stream (issue #314 Stage C)
	const size_t count = kernel::hw::pci::num_devices;
	if (count == 0) {
		resp.result = OK;
		kernel::task::reply(m, &resp);
		return;
	}

	auto buf = kernel::task::make_ool_buffer(count * sizeof(PciDeviceInfo));
	if (!buf) {
		resp.result = ERR_NO_MEMORY;
		kernel::task::reply(m, &resp);
		return;
	}

	auto* infos = static_cast<PciDeviceInfo*>(buf.get());
	for (size_t i = 0; i < count; ++i) {
		auto& device = kernel::hw::pci::devices[i];
		infos[i].vendor_id = device.vendor_id;
		infos[i].device_id = device.device_id;
		device.address(infos[i].bus_address, sizeof(infos[i].bus_address));
	}

	resp.result = OK;
	kernel::task::reply_with_ool(m, &resp, std::move(buf),
								 count * sizeof(PciDeviceInfo));
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

	t->add_msg_handler(MsgType::KERNEL_TASK_READY, handle_task_ready);
	t->add_msg_handler(MsgType::KERNEL_MEMORY_USAGE, handle_memory_usage);
	t->add_msg_handler(MsgType::KERNEL_PCI_LIST, handle_pci);

	kernel::task::process_messages(t);
}

void shell_service()
{
	// One synchronous FS_LOAD returns a kernel-owned copy of the binary as
	// an OOL move; exec_elf takes ownership and frees it once the
	// segments are loaded (issue #314 Stage C)
	Message m = { .type = MsgType::FS_LOAD, .sender = process_ids::SHELL };
	char path[6] = "shell";
	memcpy(m.data.fs.name, path, 6);

	const error_t load_err = kernel::task::call(process_ids::FS_FAT32, &m);

	// Take ownership before inspecting the result (mirrors sys_exec)
	kernel::memory::unique_kbuf<> shell_elf{ reinterpret_cast<void*>(m.ool.addr) };
	if (IS_ERR(load_err) || IS_ERR(m.result) || m.ool.size == 0 || !shell_elf) {
		LOG_ERROR("failed to load shell binary");
		while (true) {
			__asm__("hlt");
		}
	}

	CURRENT_TASK->is_initialized = true;
	exec_elf(std::move(shell_elf), "shell", nullptr);
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
