#include "ipc.hpp"
#include "../graphics/log.hpp"
#include "../types.hpp"
#include "task.hpp"

error_t send_message(task_t dst_id, const message* m)
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

	if (dst->state == TASK_WAITING) {
		schedule_task(dst_id);
	}

	dst->messages.push(*m);

	return OK;
}
