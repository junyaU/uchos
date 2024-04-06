#include "ipc.hpp"
#include "syscall.hpp"

void receive_message(message* msg)
{
	sys_ipc(msg->sender, msg->sender, msg, IPC_RECV);
}

void send_message(int task_id, const message* msg)
{
	sys_ipc(task_id, msg->sender, msg, IPC_SEND);
}

void initialize_task()
{
	message m;
	m.type = IPC_INITIALIZE_TASK;

	send_message(KERNEL_TASK_ID, &m);
}
