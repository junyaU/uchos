#include "fs/fat/fat.hpp"
#include "graphics/font.hpp"
#include "graphics/log.hpp"
#include "graphics/screen.hpp"
#include "memory/page.hpp"
#include "memory/paging.hpp"
#include "memory/slab.hpp"
#include "memory/user.hpp"
#include "point2d.hpp"
#include "syscall.hpp"
#include "task/context.hpp"
#include "task/context_switch.h"
#include "task/ipc.hpp"
#include "task/task.hpp"
#include "timers/timer.hpp"
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <fcntl.h>
#include <libs/common/message.hpp>
#include <libs/common/process_id.hpp>
#include <libs/common/types.hpp>
#include <stdint.h>
#include <sys/types.h>
#include <vector>

namespace kernel::syscall
{
ssize_t sys_read(uint64_t arg1, uint64_t arg2, uint64_t arg3)
{
	const fd_t fd = static_cast<fd_t>(arg1);
	// void __user* buf = reinterpret_cast<void*>(arg2);
	// const size_t count = static_cast<size_t>(arg3);

	kernel::task::Task* t = kernel::task::CURRENT_TASK;

	// Validate file descriptor
	if (fd < 0 || fd >= kernel::task::MAX_FDS_PER_PROCESS || t->fd_table[fd].is_unused()) {
		return ERR_INVALID_FD;
	}

	// For now, only support stdin (fd 0)
	if (fd == STDIN_FILENO) {
		// TODO: Implement actual keyboard input via IPC
		// For now, return 0 (no data available)
		return 0;
	}

	// Other file descriptors not yet supported
	return ERR_INVALID_FD;
}

ssize_t sys_write(uint64_t arg1, uint64_t arg2, uint64_t arg3)
{
	const fd_t fd = static_cast<fd_t>(arg1);
	const void __user* buf = reinterpret_cast<const void*>(arg2);
	const size_t count = static_cast<size_t>(arg3);

	kernel::task::Task* t = kernel::task::CURRENT_TASK;

	if (fd < 0 || fd >= kernel::task::MAX_FDS_PER_PROCESS || t->fd_table[fd].is_unused()) {
		return ERR_INVALID_FD;
	}

	if (fd == STDOUT_FILENO || fd == STDERR_FILENO) {
		if (t->fd_table[fd].has_name("stdout") || t->fd_table[fd].has_name("stderr")) {
			// Standard output - send to terminal
			Message m = { .type = MsgType::NOTIFY_WRITE, .sender = t->id };

			// Copy data from user space
			const size_t copy_size = count > sizeof(m.data.write_shell.buf) - 1
			                             ? sizeof(m.data.write_shell.buf) - 1
			                             : count;
			copy_from_user(m.data.write_shell.buf, buf, copy_size);
			m.data.write_shell.buf[copy_size] = '\0';

			kernel::task::send_message(process_ids::SHELL, m);

			return copy_size;
		}
		// File output - fd contains actual file info after dup2
		Message m = { .type = MsgType::FS_WRITE, .sender = t->id };
		m.data.fs.fd = fd;
		m.data.fs.len = count;

		// For small writes, use inline buffer temporarily
		if (count <= sizeof(m.data.fs.temp_buf) - 1) {
			copy_from_user(m.data.fs.temp_buf, buf, count);
			m.data.fs.temp_buf[count] = '\0';
		} else {
			// Large writes not supported currently
			LOG_ERROR("Large write not supported: %d bytes", count);
			return ERR_INVALID_ARG;
		}

		kernel::task::send_message(process_ids::FS_FAT32, m);

		// Wait for response from file system
		Message reply = kernel::task::wait_for_message(MsgType::FS_WRITE);

		// The FS process will deallocate the OOL memory
		return reply.data.fs.len;
	}

	// For file descriptors, the user process should use fs_write() through IPC
	// The kernel doesn't directly handle file writes in Phase 2
	// This allows proper IPC message handling in userland

	// For now, return an error indicating the operation is not supported at syscall level
	// User should use the file system API (fs_write) instead
	return ERR_INVALID_FD;
}

size_t sys_draw_text(uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t arg4)
{
	const char* text = reinterpret_cast<const char*>(arg1);
	const int x = arg2;
	const int y = arg3;
	const uint32_t color = arg4;

	write_string(*kernel::graphics::kscreen, { x, y }, text, color);

	return strlen(text);
}

error_t sys_fill_rect(uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t arg4, uint64_t arg5)
{
	const int x = arg1;
	const int y = arg2;
	const int width = arg3;
	const int height = arg4;
	const uint32_t color = arg5;

	kernel::graphics::kscreen->fill_rectangle(Point2D{ x, y }, Point2D{ width, height }, color);

	return OK;
}

error_t sys_time(uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t arg4)
{
	const uint64_t ms = arg1;
	const int is_periodic = arg2;
	const TimeoutAction action = static_cast<TimeoutAction>(arg3);
	const ProcessId task_id = ProcessId::from_raw(arg4);

	if (is_periodic == 1) {
		kernel::timers::ktimer->add_periodic_timer_event(ms, action, task_id);
	} else {
		kernel::timers::ktimer->add_timer_event(ms, action, task_id);
	}

	return OK;
}

error_t sys_ipc(uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t arg4)
{
	const int dest = arg1;
	Message __user& m = *reinterpret_cast<Message*>(arg3);
	const int flags = arg4;

	kernel::task::Task* t = kernel::task::CURRENT_TASK;
	t->state = kernel::task::TASK_WAITING;

	if (flags == IPC_RECV) {
		if (t->messages.empty()) {
			m = {};
			m.type = MsgType::NO_TASK;
			return OK;
		}

		t->state = kernel::task::TASK_RUNNING;

		copy_to_user(&m, &t->messages.front(), sizeof(m));
		t->messages.pop();

		return OK;
	}

	if (flags == IPC_SEND) {
		Message copy_m;
		copy_from_user(&copy_m, &m, sizeof(m));

		if (copy_m.type == MsgType::INITIALIZE_TASK) {
			copy_m.sender = t->id;
		}

		kernel::task::send_message(ProcessId::from_raw(dest), copy_m);
	}

	return OK;
}

ProcessId sys_fork(void)
{
	kernel::task::Context current_ctx;
	memset(&current_ctx, 0, sizeof(current_ctx));
	asm volatile("mov %%rdi, %0" : "=r"(current_ctx.rdi));
	get_current_context(&current_ctx);

	kernel::task::Task* t = kernel::task::CURRENT_TASK;
	if (t->parent_id.raw() != -1) {
		return ProcessId::from_raw(0);
	}

	kernel::task::Task* child = kernel::task::copy_task(t, &current_ctx);
	if (child == nullptr) {
		return ProcessId::from_raw(ERR_FORK_FAILED);
	}

	kernel::task::schedule_task(child->id);

	return child->id;
}

error_t sys_exec(uint64_t arg1, uint64_t arg2, uint64_t arg3)
{
	const char __user* path = reinterpret_cast<const char*>(arg1);
	const char __user* args = reinterpret_cast<const char*>(arg2);

	const size_t path_len = strlen(path);
	std::vector<char> copy_path(path_len + 1);
	copy_from_user(copy_path.data(), path, path_len);
	copy_path[path_len] = '\0';

	const size_t args_len = strlen(args);
	std::vector<char> copy_args(args_len + 1);
	copy_from_user(copy_args.data(), args, args_len);
	copy_args[args_len] = '\0';

	Message msg{ .type = MsgType::IPC_GET_FILE_INFO, .sender = kernel::task::CURRENT_TASK->id };
	memcpy(msg.data.fs.name, copy_path.data(), path_len + 1);
	kernel::task::send_message(process_ids::FS_FAT32, msg);

	const Message info_m = kernel::task::wait_for_message(MsgType::IPC_GET_FILE_INFO);

	auto* entry = reinterpret_cast<kernel::fs::DirectoryEntry*>(info_m.data.fs.buf);
	if (entry == nullptr) {
		return ERR_NO_FILE;
	}

	Message read_msg{ .type = MsgType::IPC_READ_FILE_DATA, .sender = kernel::task::CURRENT_TASK->id };
	read_msg.data.fs.buf = entry;
	kernel::task::send_message(process_ids::FS_FAT32, read_msg);

	const Message data_m = kernel::task::wait_for_message(MsgType::IPC_READ_FILE_DATA);
	kernel::memory::free(entry);

	// Save current FD table before cleaning page tables
	auto saved_fd_table = kernel::task::CURRENT_TASK->fd_table;

	kernel::memory::page_table_entry* current_page_table = kernel::memory::get_active_page_table();
	kernel::memory::clean_page_tables(current_page_table);

	kernel::memory::page_table_entry* new_page_table = kernel::memory::config_new_page_table();
	if (new_page_table == nullptr) {
		return ERR_NO_MEMORY;
	}

	kernel::task::CURRENT_TASK->ctx.cr3 = reinterpret_cast<uint64_t>(new_page_table);

	// Restore FD table after page table switch
	kernel::task::CURRENT_TASK->fd_table = saved_fd_table;

	// TODO: fix this
	kernel::fs::fat::execute_file(data_m.data.fs.buf, "", copy_args.data());

	return OK;
}

void sys_exit(uint64_t arg1)
{
	kernel::task::exit_task(arg1);
}

ProcessId sys_wait(uint64_t arg1)
{
	auto __user* status = reinterpret_cast<int*>(arg1);

	kernel::task::Task* t = kernel::task::CURRENT_TASK;

	while (true) {
		if (t->messages.empty()) {
			t->state = kernel::task::TASK_WAITING;
			asm("pause");
			continue;
		}

		t->state = kernel::task::TASK_RUNNING;

		Message m = t->messages.front();
		t->messages.pop();

		if (m.type == MsgType::IPC_EXIT_TASK) {
			task::send_message(process_ids::SHELL, m);
			copy_to_user(status, &m.data.exit_task.status, sizeof(int));
			return m.sender;
		}

		__asm__("cli");
		t->messages.push(m);
		__asm__("sti");
	}

	return ProcessId::from_raw(-1);
}

ProcessId sys_getpid(void)
{
	return kernel::task::CURRENT_TASK->id;
}

ProcessId sys_getppid(void)
{
	return kernel::task::CURRENT_TASK->parent_id;
}

}  // namespace kernel::syscall

extern "C" uint64_t handle_syscall(uint64_t arg1,
                                   uint64_t arg2,
                                   uint64_t arg3,
                                   uint64_t arg4,
                                   uint64_t arg5,
                                   uint64_t syscall_number)
{
	uint64_t result = 0;

	switch (syscall_number) {
		case kernel::syscall::SYS_READ:
			result = kernel::syscall::sys_read(arg1, arg2, arg3);
			break;
		case kernel::syscall::SYS_WRITE:
			result = kernel::syscall::sys_write(arg1, arg2, arg3);
			break;
		case kernel::syscall::SYS_EXIT:
			kernel::syscall::sys_exit(arg1);
			break;
		case kernel::syscall::SYS_DRAW_TEXT:
			result = kernel::syscall::sys_draw_text(arg1, arg2, arg3, arg4);
			break;
		case kernel::syscall::SYS_FILL_RECT:
			result = kernel::syscall::sys_fill_rect(arg1, arg2, arg3, arg4, arg5);
			break;
		case kernel::syscall::SYS_TIME:
			result = kernel::syscall::sys_time(arg1, arg2, arg3, arg4);
			break;
		case kernel::syscall::SYS_IPC:
			result = kernel::syscall::sys_ipc(arg1, arg2, arg3, arg4);
			break;
		case kernel::syscall::SYS_FORK:
			result = kernel::syscall::sys_fork().raw();
			break;
		case kernel::syscall::SYS_EXEC:
			result = kernel::syscall::sys_exec(arg1, arg2, arg3);
			break;
		case kernel::syscall::SYS_WAIT:
			result = kernel::syscall::sys_wait(arg1).raw();
			break;
		case kernel::syscall::SYS_GETPID:
			result = kernel::syscall::sys_getpid().raw();
			break;
		case kernel::syscall::SYS_GETPPID:
			result = kernel::syscall::sys_getppid().raw();
			break;
		default:
			LOG_ERROR("Unknown syscall number: %d", syscall_number);
			break;
	}

	return result;
}
