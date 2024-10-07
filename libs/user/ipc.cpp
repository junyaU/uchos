#include "ipc.hpp"
#include "libs/common/types.hpp"
#include "syscall.hpp"

void receive_message(message* msg)
{
	sys_ipc(msg->sender, msg->sender, msg, IPC_RECV);
}

void send_message(pid_t dst, const message* msg)
{
	sys_ipc(dst, msg->sender, msg, IPC_SEND);
}

void initialize_task()
{
	message m;
	m.type = IPC_INITIALIZE_TASK;

	send_message(KERNEL_TASK_ID, &m);
}

void deallocate_ool_memory(pid_t sender, void* addr, size_t size)
{
	message m = { .type = IPC_OOL_MEMORY_DEALLOC, .sender = sender };
	m.tool_desc.addr = addr;
	m.tool_desc.size = size;
	m.tool_desc.present = true;

	send_message(KERNEL_TASK_ID, &m);
}
