#include "ipc.hpp"
#include "syscall.hpp"

void receive_message(message* msg) { sys_ipc(0, 0, msg, IPC_RECV); }

void send_message(int task_id, const message* msg)
{
	// TODO: implement
	int shell_task_id = 4;

	sys_ipc(task_id, shell_task_id, msg, IPC_SEND);
}