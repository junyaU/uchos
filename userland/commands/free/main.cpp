#include "libs/common/message.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>
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

		message send_m = { .type = NOTIFY_WRITE, .sender = pid };

		char buf[100];
		sprintf(buf, "Used memory: %u MiB\nTotal memory: %u MiB",
				msg.data.memory_usage.used / 1024 / 1024,
				msg.data.memory_usage.total / 1024 / 1024);

		memcpy(send_m.data.write_shell.buf, buf, strlen(buf));

		send_message(SHELL_TASK_ID, &send_m);

		exit(0);
	}

	exit(0);
}
