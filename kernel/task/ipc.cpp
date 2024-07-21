#include "ipc.hpp"
#include "graphics/log.hpp"
#include "memory/paging.hpp"
#include "memory/paging_utils.h"
#include "task.hpp"
#include <cstdint>
#include <libs/common/types.hpp>

error_t send_message(pid_t dst_id, const message* m)
{
	if (dst_id == -1 || m->sender == dst_id) {
		printk(KERN_ERROR, "invalid destination task id : dest = %d, sender = %d",
			   dst_id, m->sender);
		return ERR_INVALID_ARG;
	}

	task* dst = tasks[dst_id];
	if (dst == nullptr) {
		printk(KERN_ERROR, "send_message: task %d is not found", dst_id);
		return ERR_INVALID_TASK;
	}

	if (m->tool_desc.present) {
		vaddr_t addr{ reinterpret_cast<uint64_t>(m->tool_desc.addr) };

		paddr_t src_frame =
				get_paddr(reinterpret_cast<page_table_entry*>(get_cr3()), addr);

		auto* dst_pml4 = reinterpret_cast<page_table_entry*>(dst->ctx.cr3);

		vaddr_t dst_addr = map_frame_to_vaddr(dst_pml4, src_frame);

		// TODO: fix const_cast
		const_cast<void*&>(m->tool_desc.addr) =
				reinterpret_cast<void*>(dst_addr.data + addr.part(0));
	}

	if (dst->state == TASK_WAITING) {
		schedule_task(dst_id);
	}

	dst->messages.push(*m);

	return OK;
}
