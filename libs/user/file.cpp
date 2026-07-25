#include <sys/types.h>
#include <cstring>
#include <libs/common/message.hpp>
#include <libs/common/process_id.hpp>
#include <libs/common/types.hpp>
#include <libs/user/file.hpp>
#include <libs/user/ipc.hpp>
#include <libs/user/syscall.hpp>

fd_t fs_open(const char* path, int flags)
{
	Message m = make_request(MsgType::FS_OPEN);
	memcpy(m.data.fs.name, path, strlen(path));
	m.data.fs.operation = flags;

	Message res = call(process_ids::FS_FAT32, &m);

	return IS_ERR(res.result) ? res.result : res.data.fs.fd;
}

ssize_t fs_read(fd_t fd, void* buf, size_t count)
{
	// One path only (issue #315 3b-8): the kernel resolves fd -> server
	// handle and forwards. The old direct-IPC route sent the client-local
	// fd number, which the FS no longer understands.
	return static_cast<ssize_t>(
			sys_read(fd, reinterpret_cast<uint64_t>(buf), count));
}

void fs_close(fd_t fd)
{
	Message m = make_request(MsgType::FS_CLOSE);
	m.data.fs.fd = fd;

	send_message(process_ids::FS_FAT32, &m);
}

fd_t fs_create(const char* path)
{
	Message m = make_request(MsgType::FS_MKFILE);
	memcpy(m.data.fs.name, path, strlen(path));

	Message res = call(process_ids::FS_FAT32, &m);

	return IS_ERR(res.result) ? res.result : res.data.fs.fd;
}

error_t fs_pwd(char* buf, size_t size)
{
	Message m = make_request(MsgType::FS_PWD);

	Message res = call(process_ids::FS_FAT32, &m);
	if (IS_ERR(res.result)) {
		buf[0] = '\0';
		return res.result;
	}

	memcpy(buf, res.data.fs.name, size);

	return OK;
}

ssize_t fs_write(fd_t fd, const void* buf, size_t count)
{
	// Same single path as fs_read: fd -> handle translation is the
	// kernel's job (issue #315 3b-8)
	return static_cast<ssize_t>(
			sys_write(fd, reinterpret_cast<uint64_t>(buf), count));
}

error_t fs_change_dir(char* buf, const char* path)
{
	Message m = make_request(MsgType::FS_CHANGE_DIR);
	memcpy(m.data.fs.name, path, strlen(path));
	m.data.fs.name[strlen(path)] = '\0';

	Message res = call(process_ids::FS_FAT32, &m);
	if (IS_ERR(res.result)) {
		buf[0] = '\0';
		return res.result;
	}

	memcpy(buf, res.data.fs.name, strlen(res.data.fs.name));
	buf[strlen(res.data.fs.name)] = '\0';

	return OK;
}

fd_t fs_dup2(fd_t oldfd, fd_t newfd)
{
	Message m = make_request(MsgType::FS_DUP2);
	m.data.fs.fd = oldfd;
	m.data.fs.operation = newfd;

	Message res = call(process_ids::FS_FAT32, &m);

	return IS_ERR(res.result) ? res.result : res.data.fs.fd;
}
