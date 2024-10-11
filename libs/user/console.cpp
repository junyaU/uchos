#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <libs/common/message.hpp>
#include <libs/user/console.hpp>
#include <libs/user/ipc.hpp>
#include <libs/user/syscall.hpp>

void printu(const char* format, ...)
{
	va_list args;
	va_start(args, format);
	char buffer[256];
	vsprintf(buffer, format, args);
	va_end(args);

	pid_t pid = sys_getpid();

	message m = { .type = NOTIFY_WRITE, .sender = pid, .is_end_of_message = true };
	memcpy(m.data.write_shell.buf, buffer, sizeof(buffer));

	send_message(SHELL_TASK_ID, &m);
}
