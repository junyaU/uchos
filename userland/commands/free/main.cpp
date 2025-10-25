#include <libs/common/message.hpp>
#include <libs/common/process_id.hpp>
#include <libs/common/types.hpp>
#include <libs/user/console.hpp>
#include <libs/user/ipc.hpp>
#include <libs/user/syscall.hpp>

extern "C" int main(int argc, char** argv)
{
	pid_t pid = sys_getpid();

	Message m = { .type = MsgType::IPC_MEMORY_USAGE,
				  .sender = ProcessId::from_raw(pid) };
	send_message(process_ids::KERNEL, &m);

	Message msg = wait_for_message(MsgType::IPC_MEMORY_USAGE);

	printu("Used memory: %u MiB\nTotal memory: %u MiB",
		   msg.data.memory_usage.used / 1024 / 1024,
		   msg.data.memory_usage.total / 1024 / 1024);

	return 0;
}
