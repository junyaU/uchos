#include "task/builtin.hpp"
#include <cstddef>
#include <libs/common/message.hpp>
#include <libs/common/process_id.hpp>
#include <libs/common/types.hpp>
#include <utility>
#include "hardware/pci.hpp"
#include "memory/page.hpp"
#include "task/ipc.hpp"
#include "task/task.hpp"

namespace
{
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

} // namespace kernel::task
