#include "ipc.hpp"
#include <cstddef>
#include <cstdint>
#include <libs/common/message.hpp>
#include <libs/common/process_id.hpp>
#include <libs/common/types.hpp>
#include "error.hpp"
#include "graphics/log.hpp"
#include "interrupt/irq_guard.hpp"
#include "memory/paging.hpp"
#include "memory/user.hpp"
#include "task.hpp"

namespace kernel::task
{

error_t handle_ool_memory_dealloc(const Message& m)
{
	const kernel::memory::vaddr_t addr{ reinterpret_cast<uint64_t>(
			m.tool_desc.addr) };
	const kernel::memory::vaddr_t start_addr{ addr.data - addr.part(0) };
	const size_t pages_to_unmap =
			kernel::memory::calc_required_pages(addr, m.tool_desc.size);

	return kernel::memory::unmap_frame(kernel::memory::get_active_page_table(),
									   start_addr, pages_to_unmap);
}

error_t handle_ool_memory_alloc(Message& m, Task* dst)
{
	const kernel::memory::vaddr_t src_vaddr{ reinterpret_cast<uint64_t>(
			m.tool_desc.addr) };
	const size_t num_pages =
			kernel::memory::calc_required_pages(src_vaddr, m.tool_desc.size);
	paddr_t src_paddr = kernel::memory::get_paddr(
			kernel::memory::get_active_page_table(), src_vaddr);
	int data_offset = src_vaddr.part(0);

	// kernel → userspace: the kernel is identity-mapped, so the buffer's
	// physical address is its address. map_frame_to_vaddr maps whole frames and
	// hands back a page-aligned vaddr, so the buffer's in-page offset still has
	// to be re-applied below. Zeroing it only worked while kernel OOL buffers
	// happened to be page-aligned; slab objects generally are not.
	if (!is_user_address(m.tool_desc.addr, m.tool_desc.size)) {
		src_paddr = reinterpret_cast<uint64_t>(m.tool_desc.addr);
	}

	kernel::memory::vaddr_t dst_vaddr;
	RETURN_IF_ERROR(kernel::memory::map_frame_to_vaddr(
			dst->get_page_table(), src_paddr, num_pages, &dst_vaddr));

	m.tool_desc.addr = reinterpret_cast<void*>(dst_vaddr.data + data_offset);

	return OK;
}

error_t send_message(ProcessId dst_id, Message& m)
{
	const pid_t dst_raw = dst_id.raw();
	if (dst_raw == -1 || m.sender.raw() == dst_raw) {
		LOG_ERROR_CODE(ERR_INVALID_ARG,
					   "invalid destination task id : dest = %d, sender = %d",
					   dst_raw, m.sender.raw());
		return ERR_INVALID_ARG;
	}

	Task* dst = tasks[dst_raw];
	if (dst == nullptr) {
		if (m.type != MsgType::INITIALIZE_TASK) {
			LOG_ERROR_CODE(ERR_INVALID_TASK, "task %d is not found", dst_raw);
			LOG_ERROR("message type: %d", m.type);
		}
		return ERR_INVALID_TASK;
	}

	if (m.type == MsgType::IPC_OOL_MEMORY_DEALLOC) {
		RETURN_IF_ERROR(handle_ool_memory_dealloc(m));
	} else if (m.tool_desc.present) {
		RETURN_IF_ERROR(handle_ool_memory_alloc(m, dst));
	}

	// The queue is shared with interrupt handlers; keep the wakeup check
	// and the push atomic so a receiver going to sleep cannot miss it
	const kernel::interrupt::IrqGuard guard;

	if (dst->messages.size() >= MAX_QUEUED_MESSAGES) {
		LOG_ERROR_CODE(ERR_QUEUE_FULL, "message queue full: dest = %d, type = %d",
					   dst_raw, static_cast<int>(m.type));
		return ERR_QUEUE_FULL;
	}

	if (dst->state == TASK_WAITING) {
		schedule_task(dst_id);
	}

	dst->messages.push_back(m);

	return OK;
}

} // namespace kernel::task
