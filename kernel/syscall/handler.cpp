#include "file_system/fat.hpp"
#include "graphics/font.hpp"
#include "graphics/log.hpp"
#include "graphics/screen.hpp"
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
#include <vector>
#include <fcntl.h>
#include <libs/common/message.hpp>
#include <libs/common/types.hpp>
#include <libs/common/process_id.hpp>
#include <stdint.h>

namespace kernel::syscall
{
size_t sys_draw_text(uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t arg4)
{
	const char* text = reinterpret_cast<const char*>(arg1);
	const int x = arg2;
	const int y = arg3;
	const uint32_t color = arg4;

	write_string(*kernel::graphics::kscreen, { x, y }, text, color);

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

	kernel::graphics::kscreen->fill_rectangle(point2d{ x, y }, point2d{ width, height }, color);

	return OK;
}

error_t sys_time(uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t arg4)
{
	const uint64_t ms = arg1;
	const int is_periodic = arg2;
	const timeout_action_t action = static_cast<timeout_action_t>(arg3);
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
	message __user& m = *reinterpret_cast<message*>(arg3);
	const int flags = arg4;

	kernel::task::task* t = kernel::task::CURRENT_TASK;
	t->state = kernel::task::TASK_WAITING;

	if (flags == IPC_RECV) {
		if (t->messages.empty()) {
			m = {};
			m.type = msg_t::NO_TASK;
			return OK;
		}

		t->state = kernel::task::TASK_RUNNING;

		copy_to_user(&m, &t->messages.front(), sizeof(m));
		t->messages.pop();

		return OK;
	}

	if (flags == IPC_SEND) {
		message copy_m;
		copy_from_user(&copy_m, &m, sizeof(m));

		if (copy_m.type == msg_t::INITIALIZE_TASK) {
			copy_m.sender = t->id;
		}

		kernel::task::send_message(ProcessId::from_raw(dest), copy_m);
	}

	return OK;
}

ProcessId sys_fork(void)
{
	kernel::task::context current_ctx;
	memset(&current_ctx, 0, sizeof(current_ctx));
	asm volatile("mov %%rdi, %0" : "=r"(current_ctx.rdi));
	get_current_context(&current_ctx);

	kernel::task::task* t = kernel::task::CURRENT_TASK;
	if (t->parent_id.raw() != -1) {
		return ProcessId::from_raw(0);
	}

	kernel::task::task* child = kernel::task::copy_task(t, &current_ctx);
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

	message msg{ .type = msg_t::IPC_GET_FILE_INFO, .sender = kernel::task::CURRENT_TASK->id };
	memcpy(msg.data.fs.name, copy_path.data(), path_len + 1);
	kernel::task::send_message(process_ids::FS_FAT32, msg);

	const message info_m = kernel::task::wait_for_message(msg_t::IPC_GET_FILE_INFO);

	auto* entry =
			reinterpret_cast<kernel::fs::directory_entry*>(info_m.data.fs.buf);
	if (entry == nullptr) {
		return ERR_NO_FILE;
	}

	message read_msg{ .type = msg_t::IPC_READ_FILE_DATA,
					  .sender = kernel::task::CURRENT_TASK->id };
	read_msg.data.fs.buf = entry;
	kernel::task::send_message(process_ids::FS_FAT32, read_msg);

	const message data_m = kernel::task::wait_for_message(msg_t::IPC_READ_FILE_DATA);
	kernel::memory::free(entry);

	kernel::memory::page_table_entry* current_page_table = kernel::memory::get_active_page_table();
	kernel::memory::clean_page_tables(current_page_table);

	kernel::memory::page_table_entry* new_page_table = kernel::memory::config_new_page_table();
	if (new_page_table == nullptr) {
		return ERR_NO_MEMORY;
	}

	kernel::task::CURRENT_TASK->ctx.cr3 = reinterpret_cast<uint64_t>(new_page_table);

	// TODO: fix this
	kernel::fs::fat::execute_file(data_m.data.fs.buf, "", copy_args.data());

	return OK;
}

void sys_exit(uint64_t arg1) { kernel::task::exit_task(arg1); }

ProcessId sys_wait(uint64_t arg1)
{
	auto __user* status = reinterpret_cast<int*>(arg1);

	kernel::task::task* t = kernel::task::CURRENT_TASK;

	while (true) {
		if (t->messages.empty()) {
			t->state = kernel::task::TASK_WAITING;
			asm("pause");
			continue;
		}

		t->state = kernel::task::TASK_RUNNING;

		message m = t->messages.front();
		t->messages.pop();

		if (m.type != msg_t::IPC_EXIT_TASK) {
			__asm__("cli");
			t->messages.push(m);
			__asm__("sti");
			continue;
		}

		copy_to_user(status, &m.data.exit_task.status, sizeof(int));
		return m.sender;
	}

	return ProcessId::from_raw(-1);
}

ProcessId sys_getpid(void) { return kernel::task::CURRENT_TASK->id; }

ProcessId sys_getppid(void) { return kernel::task::CURRENT_TASK->parent_id; }

} // namespace kernel::syscall

extern "C" uint64_t handle_syscall(uint64_t arg1,
								   uint64_t arg2,
								   uint64_t arg3,
								   uint64_t arg4,
								   uint64_t arg5,
								   uint64_t syscall_number)
{
	uint64_t result = 0;

	switch (syscall_number) {
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
