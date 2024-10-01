#include "ipc.hpp"
#include "graphics/log.hpp"
#include "libs/common/message.hpp"
#include "memory/paging.hpp"
#include "task.hpp"
#include <cstddef>
#include <cstdint>
#include <libs/common/types.hpp>

error_t handle_ool_memory_dealloc(const message& m)
{
	vaddr_t addr{ reinterpret_cast<uint64_t>(m.tool_desc.addr) };
	vaddr_t start_addr{ addr.data - addr.part(0) };
	size_t pages_to_unmap = calc_required_pages(addr, m.tool_desc.size);

	return unmap_frame(get_active_page_table(), start_addr, pages_to_unmap);
}

error_t handle_ool_memory_alloc(message& m, task* dst)
{
	vaddr_t src_vaddr{ reinterpret_cast<uint64_t>(m.tool_desc.addr) };
	size_t num_pages = calc_required_pages(src_vaddr, m.tool_desc.size);
	paddr_t src_paddr = get_paddr(get_active_page_table(), src_vaddr);

	vaddr_t dst_vaddr;
	ASSERT_OK(map_frame_to_vaddr(dst->get_page_table(), src_paddr, num_pages,
								 &dst_vaddr));

	m.tool_desc.addr = reinterpret_cast<void*>(dst_vaddr.data + src_vaddr.part(0));

	return OK;
}

error_t send_message(pid_t dst_id, message* m)
{
	if (dst_id == -1 || m->sender == dst_id) {
		LOG_ERROR("invalid destination task id : dest = %d, sender = %d", dst_id,
				  m->sender);
		return ERR_INVALID_ARG;
	}

	task* dst = tasks[dst_id];
	if (dst == nullptr) {
		LOG_ERROR("send_message: task %d is not found", dst_id);
		return ERR_INVALID_TASK;
	}

	if (m->type == IPC_OOL_MEMORY_DEALLOC) {
		ASSERT_OK(handle_ool_memory_dealloc(*m));
	} else if (m->tool_desc.present) {
		ASSERT_OK(handle_ool_memory_alloc(*m, dst));
	}

	if (dst->state == TASK_WAITING) {
		schedule_task(dst_id);
	}

	dst->messages.push(*m);

	return OK;
}
