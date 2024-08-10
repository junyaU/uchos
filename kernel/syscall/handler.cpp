#include "file_system/fat.hpp"
#include "graphics/font.hpp"
#include "graphics/log.hpp"
#include "graphics/screen.hpp"
#include "libs/common/message.hpp"
#include "memory/paging.hpp"
#include "memory/user.hpp"
#include "sys/_default_fcntl.h"
#include "syscall.hpp"
#include "task/context_switch.h"
#include "task/ipc.hpp"
#include "task/task.hpp"
#include "timers/timer.hpp"
#include <cerrno>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <fcntl.h>
#include <libs/common/types.hpp>
#include <memory>
#include <stdint.h>

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

	task* t = CURRENT_TASK;
	if (fd >= t->fds.size() || t->fds[fd] == nullptr) {
		return EBADF;
	}

	const size_t written = t->fds[fd]->write(buf, count);
	if (written != count) {
		return EIO;
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

	auto* entry = file_system::find_directory_entry_by_path(path);
	if (entry == nullptr) {
		if ((flags & O_CREAT) == 0) {
			printk(KERN_ERROR, "open: %s: No such file or directory\n", path);
			return NO_FD;
		}

		auto* new_entry = file_system::create_file(path);
		if (new_entry == nullptr) {
			printk(KERN_ERROR, "open: %s: No such file or directory\n", path);
			return NO_FD;
		}

		entry = new_entry;
	}

	task* t = CURRENT_TASK;
	const fd_t fd = allocate_fd(t);
	if (fd == NO_FD) {
		printk(KERN_ERROR, "open: %s: Too many open files\n", path);
		return NO_FD;
	}

	t->fds[fd] = std::make_unique<file_system::file_descriptor>(*entry);

	return fd;
}

size_t sys_draw_text(uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t arg4)
{
	const char* text = reinterpret_cast<const char*>(arg1);
	const int x = arg2;
	const int y = arg3;
	const uint32_t color = arg4;

	write_string(*kscreen, { x, y }, text, color);

	return strlen(text);
}

error_t sys_fill_rect(uint64_t arg1,
					  uint64_t arg2,
					  uint64_t arg3,
					  uint64_t arg4,
					  uint64_t arg5)
{
	const int x = arg1;
	const int y = arg2;
	const int width = arg3;
	const int height = arg4;
	const uint32_t color = arg5;

	kscreen->fill_rectangle(Point2D{ x, y }, Point2D{ width, height }, color);

	return OK;
}

error_t sys_time(uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t arg4)
{
	const uint64_t ms = arg1;
	const int is_periodic = arg2;
	const timeout_action_t action = static_cast<timeout_action_t>(arg3);
	const pid_t task_id = arg4;

	if (is_periodic == 1) {
		ktimer->add_periodic_timer_event(ms, action, task_id);
	} else {
		ktimer->add_timer_event(ms, action, task_id);
	}

	return OK;
}

error_t sys_ipc(uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t arg4)
{
	const int dest = arg1;
	const int src = arg2;
	message __user& m = *reinterpret_cast<message*>(arg3);
	const int flags = arg4;

	task* t = CURRENT_TASK;
	t->state = TASK_WAITING;

	if (flags == IPC_RECV) {
		if (t->messages.empty()) {
			memset(&m, 0, sizeof(m));
			m.type = NO_TASK;
			return OK;
		}

		t->state = TASK_RUNNING;

		copy_to_user(&m, &t->messages.front(), sizeof(m));
		t->messages.pop();

		return OK;
	}

	if (flags == IPC_SEND) {
		message copy_m;
		copy_from_user(&copy_m, &m, sizeof(m));

		if (copy_m.type == IPC_INITIALIZE_TASK) {
			copy_m.sender = t->id;
		}

		send_message(dest, &copy_m);
	}

	return OK;
}

pid_t sys_fork(void)
{
	context current_ctx;
	memset(&current_ctx, 0, sizeof(current_ctx));
	__asm__ volatile("mov %%rdi, %0" : "=r"(current_ctx.rdi));
	get_current_context(&current_ctx);

	task* t = CURRENT_TASK;
	if (t->parent_id != -1) {
		return 0;
	}

	task* child = copy_task(t, &current_ctx);
	if (child == nullptr) {
		return ERR_FORK_FAILED;
	}

	schedule_task(child->id);

	return child->id;
}

error_t sys_exec(uint64_t arg1, uint64_t arg2, uint64_t arg3)
{
	const char __user* path = reinterpret_cast<const char*>(arg1);
	const char __user* args = reinterpret_cast<const char*>(arg2);

	size_t path_len = strlen(path);
	char copy_path[path_len + 1];
	copy_from_user(copy_path, path, path_len);
	copy_path[path_len] = '\0';

	size_t args_len = strlen(args);
	char copy_args[args_len + 1];
	copy_from_user(copy_args, args, args_len);
	copy_args[args_len] = '\0';

	message msg{ .type = IPC_GET_FILE_INFO, .sender = CURRENT_TASK->id };
	memcpy(msg.data.fs_op.path, copy_path, path_len + 1);
	send_message(FS_FAT32_TASK_ID, &msg);

	message info_m = wait_for_message(IPC_GET_FILE_INFO);

	auto* entry =
			reinterpret_cast<file_system::directory_entry*>(info_m.data.fs_op.buf);
	if (entry == nullptr) {
		return ERR_NO_FILE;
	}

	message read_msg{ .type = IPC_READ_FILE_DATA, .sender = CURRENT_TASK->id };
	read_msg.data.fs_op.buf = info_m.data.fs_op.buf;
	send_message(FS_FAT32_TASK_ID, &read_msg);

	message data_m = wait_for_message(IPC_READ_FILE_DATA);

	page_table_entry* current_page_table = get_active_page_table();
	clean_page_tables(current_page_table);

	page_table_entry* new_page_table = config_new_page_table();
	if (new_page_table == nullptr) {
		return ERR_NO_MEMORY;
	}

	CURRENT_TASK->ctx.cr3 = reinterpret_cast<uint64_t>(new_page_table);

	file_system::execute_file_v2(data_m.data.fs_op.buf, "echo", copy_args);

	return OK;
}

void sys_exit(uint64_t arg1) { exit_task(arg1); }

pid_t sys_wait(uint64_t arg1)
{
	auto __user* status = reinterpret_cast<int*>(arg1);

	task* t = CURRENT_TASK;

	while (true) {
		if (t->messages.empty()) {
			t->state = TASK_WAITING;
			continue;
		}

		t->state = TASK_RUNNING;

		message m = t->messages.front();
		t->messages.pop();

		if (m.type != IPC_EXIT_TASK) {
			__asm__("cli");
			t->messages.push(m);
			__asm__("sti");
			continue;
		}

		copy_to_user(status, &m.data.exit_task.status, sizeof(int));
		return m.sender;
	}

	return NO_TASK;
}

pid_t sys_getpid(void) { return CURRENT_TASK->id; }

pid_t sys_getppid(void) { return CURRENT_TASK->parent_id; }

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
			sys_exit(arg1);
			break;
		case SYS_DRAW_TEXT:
			result = sys_draw_text(arg1, arg2, arg3, arg4);
			break;
		case SYS_FILL_RECT:
			result = sys_fill_rect(arg1, arg2, arg3, arg4, arg5);
			break;
		case SYS_TIME:
			result = sys_time(arg1, arg2, arg3, arg4);
			break;
		case SYS_IPC:
			result = sys_ipc(arg1, arg2, arg3, arg4);
			break;
		case SYS_FORK:
			result = sys_fork();
			break;
		case SYS_EXEC:
			result = sys_exec(arg1, arg2, arg3);
			break;
		case SYS_WAIT:
			result = sys_wait(arg1);
			break;
		case SYS_GETPID:
			result = sys_getpid();
			break;
		case SYS_GETPPID:
			result = sys_getppid();
			break;
		default:
			printk(KERN_ERROR, "Unknown syscall number: %d", syscall_number);
			break;
	}

	return result;
}
} // namespace syscall
