#include <algorithm>
#include <cstring>
#include <libs/common/message.hpp>
#include <libs/common/process_id.hpp>
#include <libs/common/types.hpp>
#include <libs/user/file.hpp>
#include <libs/user/ipc.hpp>

fd_t fs_open(const char* path, int flags)
{
	Message m = make_request(MsgType::FS_OPEN);
	memcpy(m.data.fs.name, path, strlen(path));
	m.data.fs.operation = flags;

	Message res = call(process_ids::FS_FAT32, &m);

	return IS_ERR(res.result) ? -1 : res.data.fs.fd;
}

size_t fs_read(fd_t fd, void* buf, size_t count)
{
	Message m = make_request(MsgType::FS_READ);
	m.data.fs.fd = fd;
	m.data.fs.len = count;

	Message res = call(process_ids::FS_FAT32, &m);
	if (IS_ERR(res.result) || !res.tool_desc.present) {
		return 0;
	}

	const size_t copy_len = std::min(count, res.tool_desc.size);
	memcpy(buf, res.tool_desc.addr, copy_len);

	deallocate_ool_memory(m.sender, res.tool_desc.addr, res.tool_desc.size);

	return copy_len;
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

	return IS_ERR(res.result) ? -1 : res.data.fs.fd;
}

void fs_pwd(char* buf, size_t size)
{
	Message m = make_request(MsgType::FS_PWD);

	Message res = call(process_ids::FS_FAT32, &m);
	if (IS_ERR(res.result)) {
		buf[0] = '\0';
		return;
	}

	memcpy(buf, res.data.fs.name, size);
}

size_t fs_write(fd_t fd, const void* buf, size_t count)
{
	Message m = make_request(MsgType::FS_WRITE);
	m.data.fs.fd = fd;
	m.data.fs.len = count;

	// Small writes ride the inline buffer; anything larger moves to OOL
	// transfer with issue #314 Stage C. (This used to memcpy through the
	// uninitialized fs.buf pointer with an 8-byte threshold.)
	if (count > sizeof(m.data.fs.temp_buf) - 1) {
		return 0;
	}
	memcpy(m.data.fs.temp_buf, buf, count);
	m.data.fs.temp_buf[count] = '\0';

	Message res = call(process_ids::FS_FAT32, &m);
	if (IS_ERR(res.result)) {
		return 0;
	}

	return res.data.fs.len;
}

void fs_change_dir(char* buf, const char* path)
{
	Message m = make_request(MsgType::FS_CHANGE_DIR);
	memcpy(m.data.fs.name, path, strlen(path));
	m.data.fs.name[strlen(path)] = '\0';

	Message res = call(process_ids::FS_FAT32, &m);
	if (IS_ERR(res.result)) {
		buf[0] = '\0';
		return;
	}

	memcpy(buf, res.data.fs.name, strlen(res.data.fs.name));
	buf[strlen(res.data.fs.name)] = '\0';
}

int fs_dup2(fd_t oldfd, fd_t newfd)
{
	Message m = make_request(MsgType::FS_DUP2);
	m.data.fs.fd = oldfd;
	m.data.fs.operation = newfd;

	Message res = call(process_ids::FS_FAT32, &m);

	return IS_ERR(res.result) ? -1 : res.data.fs.fd;
}
