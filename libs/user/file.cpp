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
	Message m = { .type = MsgType::FS_OPEN, .sender = pid };
	memcpy(m.data.fs.name, path, strlen(path));
	m.data.fs.operation = flags;

	send_message(process_ids::FS_FAT32, &m);

	Message res = wait_for_message(MsgType::FS_OPEN);

	return res.data.fs.fd;
}

size_t fs_read(fd_t fd, void* buf, size_t count)
{
	ProcessId pid = ProcessId::from_raw(sys_getpid());
	Message m = { .type = MsgType::FS_READ, .sender = pid };
	m.data.fs.fd = fd;
	m.data.fs.len = count;

	send_message(process_ids::FS_FAT32, &m);

	Message res = wait_for_message(MsgType::FS_READ);

	memcpy(buf, res.tool_desc.addr, res.tool_desc.size);

	deallocate_ool_memory(pid, res.tool_desc.addr, res.tool_desc.size);

	return res.tool_desc.size;
}

void fs_close(fd_t fd)
{
	ProcessId pid = ProcessId::from_raw(sys_getpid());
	Message m = { .type = MsgType::FS_CLOSE, .sender = pid };
	m.data.fs.fd = fd;

	send_message(process_ids::FS_FAT32, &m);
}

fd_t fs_create(const char* path)
{
	ProcessId pid = ProcessId::from_raw(sys_getpid());
	Message m = { .type = MsgType::FS_MKFILE, .sender = pid };
	memcpy(m.data.fs.name, path, strlen(path));

	send_message(process_ids::FS_FAT32, &m);

	Message res = wait_for_message(MsgType::FS_MKFILE);

	return res.data.fs.fd;
}

void fs_pwd(char* buf, size_t size)
{
	ProcessId pid = ProcessId::from_raw(sys_getpid());
	Message m = { .type = MsgType::FS_PWD, .sender = pid };

	send_message(process_ids::FS_FAT32, &m);

	Message res = wait_for_message(MsgType::FS_PWD);

	memcpy(buf, res.data.fs.name, size);
}

size_t fs_write(fd_t fd, const void* buf, size_t count)
{
	ProcessId pid = ProcessId::from_raw(sys_getpid());
	Message m = { .type = MsgType::FS_WRITE, .sender = pid };
	m.data.fs.fd = fd;
	m.data.fs.len = count;

	// For small writes, use inline buffer
	if (count <= sizeof(m.data.fs.buf)) {
		memcpy(m.data.fs.buf, buf, count);
	} else {
		// For larger writes, use OOL (out-of-line) memory
		// This would require allocating OOL memory - not implemented in Phase 1
		return 0;
	}

	send_message(process_ids::FS_FAT32, &m);

	Message res = wait_for_message(MsgType::FS_WRITE);

	return res.data.fs.len;
}

void fs_change_dir(char* buf, const char* path)
{
	ProcessId pid = ProcessId::from_raw(sys_getpid());
	Message m = { .type = MsgType::FS_CHANGE_DIR, .sender = pid };
	memcpy(m.data.fs.name, path, strlen(path));
	m.data.fs.name[strlen(path)] = '\0';

	send_message(process_ids::FS_FAT32, &m);

	Message res = wait_for_message(MsgType::FS_CHANGE_DIR);
	if (res.data.fs.result == -1) {
		buf[0] = '\0';
		return;
	}

	memcpy(buf, res.data.fs.name, strlen(res.data.fs.name));
	buf[strlen(res.data.fs.name)] = '\0';

	send_message(process_ids::SHELL, &res);
}

int fs_dup2(fd_t oldfd, fd_t newfd)
{
	ProcessId pid = ProcessId::from_raw(sys_getpid());
	Message m = { .type = MsgType::FS_DUP2, .sender = pid };
	m.data.fs.fd = oldfd;
	m.data.fs.operation = newfd;

	send_message(process_ids::FS_FAT32, &m);

	Message res = wait_for_message(MsgType::FS_DUP2);

	return res.data.fs.result;
}
