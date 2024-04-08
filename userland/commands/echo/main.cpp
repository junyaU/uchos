#include <cstdlib>
#include <cstring>
#include <libs/common/types.hpp>
#include <libs/user/ipc.hpp>
#include <libs/user/syscall.hpp>

extern "C" int main(int argc, char** argv)
{
	pid_t pid = sys_getpid();
	message m;
	m.sender = pid;
	m.type = NOTIFY_WRITE;
	m.data.write_shell.is_end_of_message = true;

	if (argv[1] != nullptr) {
		memcpy(m.data.write_shell.buf, argv[1], strlen(argv[1]));
	}

	send_message(SHELL_TASK_ID, &m);

	exit(0);
}
