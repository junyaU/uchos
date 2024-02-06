#include "ipc.hpp"
#include "../graphics/terminal.hpp"
#include "../types.hpp"
#include "task.hpp"

error_t send_message(task_t dst_id, const message* m)
{
	if (dst_id == -1) {
		main_terminal->errorf("invalid destination task id: %d", dst_id);
		return ERR_INVALID_ARG;
	}

	task* dst = tasks[dst_id];
	if (dst == nullptr) {
		main_terminal->errorf("task %d is not found", dst_id);
		return ERR_INVALID_TASK;
	}

	dst->messages.push(*m);

	return OK;
}
