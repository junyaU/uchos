#include "ipc.hpp"
#include <libs/common/message.hpp>
#include <libs/common/process_id.hpp>
#include <libs/common/types.hpp>
#include "syscall.hpp"

void receive_message(Message* msg)
{
	sys_ipc(msg->sender.raw(), msg->sender.raw(), msg, IPC_RECV);
}

void send_message(ProcessId dst, const Message* msg)
{
	sys_ipc(dst.raw(), msg->sender.raw(), msg, IPC_SEND);
}

Message wait_for_message(MsgType type)
{
	Message m;
	do {
		receive_message(&m);
	} while (m.type != type);

	return m;
}

Message make_request(MsgType type)
{
	return Message{ .type = type, .sender = ProcessId::from_raw(sys_getpid()) };
}

Message call(ProcessId dst, Message* msg)
{
	send_message(dst, msg);
	return wait_for_message(msg->type);
}

void initialize_task()
{
	Message m = make_request(MsgType::FS_REGISTER_PATH);
	call(process_ids::FS_FAT32, &m);

	m.type = MsgType::INITIALIZE_TASK;
	send_message(process_ids::KERNEL, &m);
}

void deallocate_ool_memory(ProcessId sender, void* addr, size_t size)
{
	Message m = { .type = MsgType::IPC_OOL_MEMORY_DEALLOC, .sender = sender };
	m.tool_desc.addr = addr;
	m.tool_desc.size = size;
	m.tool_desc.present = true;

	send_message(process_ids::KERNEL, &m);
}
