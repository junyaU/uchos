#include "libs/common/message.hpp"
#include "libs/user/console.hpp"
#include <cstdlib>
#include <libs/common/types.hpp>
#include <libs/user/ipc.hpp>
#include <libs/user/syscall.hpp>

extern "C" int main(int argc, char** argv)
{
	pid_t pid = sys_getpid();

	message m = { .type = IPC_MEMORY_USAGE, .sender = pid };

	send_message(KERNEL_TASK_ID, &m);

	message msg;
	while (true) {
		receive_message(&msg);
		if (msg.type == NO_TASK) {
			continue;
		}

		if (msg.type != IPC_MEMORY_USAGE) {
			continue;
		}

		printu("Used memory: %u MiB\nTotal memory: %u MiB",
			   msg.data.memory_usage.used / 1024 / 1024,
			   msg.data.memory_usage.total / 1024 / 1024);

		exit(0);
	}

	exit(0);
}
