#include "task/builtin.hpp"
#include <cstddef>
#include <cstring>
#include <libs/common/file_descriptor.hpp>
#include <libs/common/message.hpp>
#include <libs/common/process_id.hpp>
#include <libs/common/types.hpp>
#include <utility>
#include "fs/file_descriptor.hpp"
#include "hardware/pci.hpp"
#include "log/log.hpp"
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

/**
 * @brief Resolve the client of an FD_* request, enforcing who may ask
 *
 * Only the file system service edits other tasks' fd tables. With a single
 * FS a fixed id is the simplest correct rule; a per-service permission can
 * move into the boot manifest once a second routing service exists. The
 * sender is trustworthy because call() stamps it.
 *
 * @param m The FD_* request
 * @param resp Reply under construction; its result is set on failure
 * @return The client task, or nullptr when the request must be refused
 */
kernel::task::Task* resolve_fd_client(const Message& m, Message* resp)
{
	if (m.sender != process_ids::FS_FAT32) {
		LOG_ERROR("task %d may not edit fd tables", m.sender.raw());
		resp->result = ERR_NOT_PERMITTED;
		return nullptr;
	}

	kernel::task::Task* client =
			kernel::task::get_task(ProcessId::from_raw(m.data.fd_route.client));
	if (client == nullptr) {
		resp->result = ERR_INVALID_TASK;
		return nullptr;
	}

	return client;
}

void handle_fd_install(const Message& m)
{
	Message resp = { .type = MsgType::FD_INSTALL, .sender = process_ids::KERNEL };
	resp.data.fd_route.fd = NO_FD;

	kernel::task::Task* client = resolve_fd_client(m, &resp);
	if (client != nullptr) {
		// The label may arrive unterminated; the entry keeps at most 12 chars
		char name[sizeof(m.data.fd_route.name)];
		memcpy(name, m.data.fd_route.name, sizeof(name));
		name[sizeof(name) - 1] = '\0';

		const fd_t fd = kernel::fs::allocate_process_fd(
				client->fd_table.data(), kernel::task::MAX_FDS_PER_PROCESS, name,
				m.data.fd_route.handle, m.sender);
		resp.data.fd_route.fd = fd;
		resp.result = fd < 0 ? ERR_INVALID_FD : OK;
	}

	kernel::task::reply(m, &resp);
}

void handle_fd_remove(const Message& m)
{
	Message resp = { .type = MsgType::FD_REMOVE, .sender = process_ids::KERNEL };

	kernel::task::Task* client = resolve_fd_client(m, &resp);
	if (client != nullptr) {
		resp.result = kernel::fs::remove_owned_fd(
				client->fd_table.data(), kernel::task::MAX_FDS_PER_PROCESS,
				m.data.fd_route.fd, m.sender, &resp.data.fd_route.handle);
	}

	kernel::task::reply(m, &resp);
}

void handle_fd_dup(const Message& m)
{
	Message resp = { .type = MsgType::FD_DUP, .sender = process_ids::KERNEL };

	kernel::task::Task* client = resolve_fd_client(m, &resp);
	if (client != nullptr) {
		resp.result = kernel::fs::dup_owned_fd(
				client->fd_table.data(), kernel::task::MAX_FDS_PER_PROCESS,
				m.data.fd_route.fd, m.data.fd_route.newfd, m.sender,
				&resp.data.fd_route.handle, &resp.data.fd_route.replaced_handle);
	}

	kernel::task::reply(m, &resp);
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
	t->add_msg_handler(MsgType::FD_INSTALL, handle_fd_install);
	t->add_msg_handler(MsgType::FD_REMOVE, handle_fd_remove);
	t->add_msg_handler(MsgType::FD_DUP, handle_fd_dup);

	kernel::task::process_messages(t);
}

} // namespace kernel::task
