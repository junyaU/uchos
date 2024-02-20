#include "file_system/fat.hpp"
#include "file_system/file_descriptor.hpp"
#include "graphics/terminal.hpp"
#include "sys/_default_fcntl.h"
#include "syscall/syscall.hpp"
#include "task/task.hpp"
#include "types.hpp"
#include <cerrno>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <fcntl.h>
#include <memory>

namespace syscall
{
size_t sys_read(uint64_t arg1, uint64_t arg2, uint64_t arg3)
{
	const auto fd = arg1;
	auto* buf = reinterpret_cast<char*>(arg2);
	const auto count = arg3;

	task* t = CURRENT_TASK;
	if (fd >= t->fds.size() || t->fds[fd] == nullptr) {
		return 0;
	}

	return t->fds[fd]->read(buf, count);
}

error_t sys_write(uint64_t arg1, uint64_t arg2, uint64_t arg3)
{
	const auto fd = arg1;
	const auto* buf = reinterpret_cast<const char*>(arg2);
	const auto count = arg3;

	if (count > 1024) {
		return E2BIG;
	}

	if (fd == 1) {
		main_terminal->print(buf);
	}

	return OK;
}

fd_t sys_open(uint64_t arg1, uint64_t arg2)
{
	const char* path = reinterpret_cast<const char*>(arg1);
	const int flags = arg2;

	if (path == nullptr) {
		return NO_FD;
	}

	if (strncmp(path, "/dev/stdin", 10) == 0) {
		return STDIN_FILENO;
	}

	if ((flags & O_ACCMODE) == O_WRONLY) {
		return NO_FD;
	}

	auto* entry = file_system::find_directory_entry_by_path(path);
	if (entry == nullptr) {
		auto* new_entry = file_system::create_file(path);
		if (new_entry == nullptr) {
			main_terminal->printf("open: %s: No such file or directory\n", path);
			return NO_FD;
		}

		entry = new_entry;
	}

	task* t = CURRENT_TASK;
	const fd_t fd = allocate_fd(t);
	if (fd == NO_FD) {
		main_terminal->printf("open: %s: Too many open files\n", path);
		return NO_FD;
	}

	t->fds[fd] = std::make_unique<file_system::file_descriptor>(*entry);

	return fd;
}

uint64_t sys_exit()
{
	task* t = CURRENT_TASK;
	return t->kernel_stack_top;
}

extern "C" uint64_t handle_syscall(uint64_t arg1,
								   uint64_t arg2,
								   uint64_t arg3,
								   uint64_t arg4,
								   uint64_t arg5,
								   uint64_t syscall_number)
{
	uint64_t result = 0;

	switch (syscall_number) {
		case SYS_READ:
			result = sys_read(arg1, arg2, arg3);
			break;
		case SYS_WRITE:
			sys_write(arg1, arg2, arg3);
			break;
		case SYS_OPEN:
			result = sys_open(arg1, arg2);
			break;
		case SYS_EXIT:
			result = sys_exit();
			break;
		default:
			printk(KERN_ERROR, "Unknown syscall number: %d", syscall_number);
			break;
	}

	return result;
}
} // namespace syscall
