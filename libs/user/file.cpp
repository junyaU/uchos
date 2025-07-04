#include <cstring>
#include <libs/common/message.hpp>
#include <libs/common/process_id.hpp>
#include <libs/common/types.hpp>
#include <libs/user/file.hpp>
#include <libs/user/ipc.hpp>
#include <libs/user/syscall.hpp>

fd_t fs_open(const char* path, int flags)
{
	ProcessId pid = ProcessId::from_raw(sys_getpid());
	message m = { .type = msg_t::FS_OPEN, .sender = pid };
	memcpy(m.data.fs.name, path, strlen(path));
	m.data.fs.operation = flags;

	send_message(process_ids::FS_FAT32, &m);

	message res = wait_for_message(msg_t::FS_OPEN);

	return res.data.fs.fd;
}

size_t fs_read(fd_t fd, void* buf, size_t count)
{
	ProcessId pid = ProcessId::from_raw(sys_getpid());
	message m = { .type = msg_t::FS_READ, .sender = pid };
	m.data.fs.fd = fd;
	m.data.fs.len = count;

	send_message(process_ids::FS_FAT32, &m);

	message res = wait_for_message(msg_t::FS_READ);

	memcpy(buf, res.tool_desc.addr, res.tool_desc.size);

	deallocate_ool_memory(pid, res.tool_desc.addr, res.tool_desc.size);

	return res.tool_desc.size;
}

void fs_close(fd_t fd)
{
	ProcessId pid = ProcessId::from_raw(sys_getpid());
	message m = { .type = msg_t::FS_CLOSE, .sender = pid };
	m.data.fs.fd = fd;

	send_message(process_ids::FS_FAT32, &m);
}

fd_t fs_create(const char* path)
{
	ProcessId pid = ProcessId::from_raw(sys_getpid());
	message m = { .type = msg_t::FS_MKFILE, .sender = pid };
	memcpy(m.data.fs.name, path, strlen(path));

	send_message(process_ids::FS_FAT32, &m);

	message res = wait_for_message(msg_t::FS_MKFILE);

	return res.data.fs.fd;
}

void fs_pwd(char* buf, size_t size)
{
	ProcessId pid = ProcessId::from_raw(sys_getpid());
	message m = { .type = msg_t::FS_PWD, .sender = pid };

	send_message(process_ids::FS_FAT32, &m);

	message res = wait_for_message(msg_t::FS_PWD);

	memcpy(buf, res.data.fs.name, size);
}
