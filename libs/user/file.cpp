#include <cstring>
#include <libs/common/message.hpp>
#include <libs/common/types.hpp>
#include <libs/user/file.hpp>
#include <libs/user/ipc.hpp>
#include <libs/user/syscall.hpp>

fd_t fs_open(const char* path, int flags)
{
	pid_t pid = sys_getpid();
	message m = { .type = msg_t::FS_OPEN, .sender = pid };
	memcpy(m.data.fs_op.path, path, strlen(path));
	m.data.fs_op.operation = flags;

	send_message(FS_FAT32_TASK_ID, &m);

	message res = wait_for_message(msg_t::FS_OPEN);

	return res.data.fs_op.fd;
}
