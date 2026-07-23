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

Message make_request(MsgType type)
{
	return Message{ .type = type, .sender = ProcessId::from_raw(sys_getpid()) };
}

Message call(ProcessId dst, Message* msg)
{
	// One correlation-matched RPC in a single syscall (issue #314 Stage B):
	// the kernel pairs the reply by id and overwrites *msg with it, so a
	// same-typed message can never be mistaken for the answer.
	sys_ipc(dst.raw(), msg->sender.raw(), msg, IPC_CALL);
	return *msg;
}

void initialize_task()
{
	Message m = make_request(MsgType::FS_REGISTER_PATH);
	call(process_ids::FS_FAT32, &m);

	m.type = MsgType::KERNEL_TASK_READY;
	send_message(process_ids::KERNEL, &m);
}

void ool_release(const void* addr)
{
	// The region is named by its mapped address; the kernel looks it up in
	// this task's region table, unmaps it and frees the pages (issue #314
	// Stage C)
	Message m = {};
	m.ool.addr = reinterpret_cast<uint64_t>(addr);

	sys_ipc(-1, -1, &m, IPC_OOL_RELEASE);
}
