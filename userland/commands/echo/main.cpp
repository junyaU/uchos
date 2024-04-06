#include <../../libs/common/types.hpp>
#include <../../libs/user/ipc.hpp>
#include <../../libs/user/syscall.hpp>
#include <cstdlib>
#include <cstring>

extern "C" int main(int argc, char** argv)
{
	message m;
	m.sender = CHILD_TASK;
	m.type = NOTIFY_WRITE;
	m.data.write_shell.is_end_of_message = true;
	memcpy(m.data.write_shell.buf, argv[1], strlen(argv[1]));

	send_message(SHELL_TASK_ID, &m);

	exit(0);
}
