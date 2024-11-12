#include "ipc.hpp"
#include "syscall.hpp"
#include <libs/common/message.hpp>
#include <libs/common/types.hpp>

void receive_message(message* msg)
{
	sys_ipc(msg->sender, msg->sender, msg, IPC_RECV);
}

void send_message(pid_t dst, const message* msg)
{
	sys_ipc(dst, msg->sender, msg, IPC_SEND);
}

message wait_for_message(msg_t type)
{
	message m;
	do {
		receive_message(&m);
	} while (m.type != type);

	return m;
}

void initialize_task()
{
	message m;

	m.type = msg_t::INITIALIZE_TASK;
	send_message(KERNEL_TASK_ID, &m);

	m.type = msg_t::FS_REGISTER_PATH;
	send_message(FS_FAT32_TASK_ID, &m);
}

void deallocate_ool_memory(pid_t sender, void* addr, size_t size)
{
	message m = { .type = msg_t::IPC_OOL_MEMORY_DEALLOC, .sender = sender };
	m.tool_desc.addr = addr;
	m.tool_desc.size = size;
	m.tool_desc.present = true;

	send_message(KERNEL_TASK_ID, &m);
}
