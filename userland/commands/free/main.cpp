#include <libs/common/message.hpp>
#include <libs/common/types.hpp>
#include <libs/user/console.hpp>
#include <libs/user/ipc.hpp>
#include <libs/user/syscall.hpp>

extern "C" int main(int argc, char** argv)
{
	pid_t pid = sys_getpid();

	message m = { .type = msg_t::IPC_MEMORY_USAGE, .sender = pid };
	send_message(KERNEL_TASK_ID, &m);

	message msg = wait_for_message(msg_t::IPC_MEMORY_USAGE);

	printu("Used memory: %u MiB\nTotal memory: %u MiB",
		   msg.data.memory_usage.used / 1024 / 1024,
		   msg.data.memory_usage.total / 1024 / 1024);

	return 0;
}
