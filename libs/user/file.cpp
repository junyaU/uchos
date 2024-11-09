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

size_t fs_read(fd_t fd, void* buf, size_t count)
{
	pid_t pid = sys_getpid();
	message m = { .type = msg_t::FS_READ, .sender = pid };
	m.data.fs_op.fd = fd;
	m.data.fs_op.len = count;

	send_message(FS_FAT32_TASK_ID, &m);

	message res = wait_for_message(msg_t::FS_READ);

	memcpy(buf, res.tool_desc.addr, res.tool_desc.size);

	deallocate_ool_memory(pid, res.tool_desc.addr, res.tool_desc.size);

	return res.tool_desc.size;
}

void fs_close(fd_t fd)
{
	pid_t pid = sys_getpid();
	message m = { .type = msg_t::FS_CLOSE, .sender = pid };
	m.data.fs_op.fd = fd;

	send_message(FS_FAT32_TASK_ID, &m);
}

fd_t fs_create(const char* path)
{
	pid_t pid = sys_getpid();
	message m = { .type = msg_t::FS_MKFILE, .sender = pid };
	memcpy(m.data.fs_op.path, path, strlen(path));

	send_message(FS_FAT32_TASK_ID, &m);

	message res = wait_for_message(msg_t::FS_MKFILE);

	return res.data.fs_op.fd;
}
