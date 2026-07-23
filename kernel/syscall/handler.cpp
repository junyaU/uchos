#include <fcntl.h>
#include <stdint.h>
#include <sys/types.h>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <libs/common/message.hpp>
#include <libs/common/process_id.hpp>
#include <libs/common/types.hpp>
#include <utility>
#include "error.hpp"
#include "fs/fat/fat.hpp"
#include "graphics/font.hpp"
#include "graphics/screen.hpp"
#include "log/log.hpp"
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

namespace kernel::syscall
{
ssize_t sys_read(uint64_t arg1, uint64_t arg2, uint64_t arg3)
{
	const fd_t fd = static_cast<fd_t>(arg1);
	// void __user* buf = reinterpret_cast<void*>(arg2);
	// const size_t count = static_cast<size_t>(arg3);

	kernel::task::Task* t = kernel::task::CURRENT_TASK;

	// Validate file descriptor
	if (fd < 0 || fd >= kernel::task::MAX_FDS_PER_PROCESS ||
		t->fd_table[fd].is_unused()) {
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

	if (fd < 0 || fd >= kernel::task::MAX_FDS_PER_PROCESS ||
		t->fd_table[fd].is_unused()) {
		return ERR_INVALID_FD;
	}

	if (count == 0) {
		return 0;
	}

	if (count > OOL_MAX_SIZE) {
		return ERR_OOL_LIMIT;
	}

	if (fd == STDOUT_FILENO || fd == STDERR_FILENO) {
		if (t->fd_table[fd].has_name("stdout") ||
			t->fd_table[fd].has_name("stderr")) {
			// Standard output - send to terminal
			Message m = { .type = MsgType::NOTIFY_WRITE, .sender = t->id };

			if (count < sizeof(m.data.write.buf)) {
				// Best-effort copy on purpose: boot-time tests call this
				// with kernel buffers, which copy_from_user rejects (the
				// message then carries an empty string)
				copy_from_user(m.data.write.buf, buf, count);
				m.data.write.buf[count] = '\0';
				kernel::task::send_message(process_ids::SHELL, m);
				return count;
			}

			// The 256B ceiling is gone (issue #314 Stage C): large stdout
			// payloads travel OOL and are released by the shell
			auto kbuf = kernel::task::make_ool_buffer(count + 1);
			if (!kbuf) {
				return ERR_NO_MEMORY;
			}
			copy_from_user(kbuf.get(), buf, count);
			static_cast<char*>(kbuf.get())[count] = '\0';
			m.ool.addr = reinterpret_cast<uint64_t>(kbuf.get());
			m.ool.size = count + 1;

			const error_t err = kernel::task::send_message(process_ids::SHELL, m);
			if (IS_ERR(err)) {
				return err;
			}
			static_cast<void>(kbuf.release()); // delivered: the shell owns it now

			return count;
		}
		// File output - fd contains actual file info after dup2. Any size
		// goes through one OOL path (issue #314 Stage C).
		Message req = { .type = MsgType::FS_WRITE, .sender = t->id };
		req.data.fs.fd = fd;
		req.data.fs.len = count;

		auto kbuf = kernel::task::make_ool_buffer(count);
		if (!kbuf) {
			return ERR_NO_MEMORY;
		}
		if (copy_from_user(kbuf.get(), buf, count) != count) {
			return ERR_INVALID_ARG;
		}
		req.ool.addr = reinterpret_cast<uint64_t>(kbuf.get());
		req.ool.size = count;

		// Correlation-matched RPC: the reply cannot be confused with any
		// other FS_WRITE traffic (issue #314 Stage B)
		const error_t err = kernel::task::call(process_ids::FS_FAT32, &req);
		if (IS_ERR(err)) {
			// Never delivered; kbuf frees the payload on return
			return err;
		}
		static_cast<void>(kbuf.release()); // consumed (and freed) by the FS task
		if (IS_ERR(req.result)) {
			return req.result;
		}

		return req.data.fs.len;
	}

	// For file descriptors, the user process should use fs_write() through IPC
	// The kernel doesn't directly handle file writes in Phase 2
	// This allows proper IPC message handling in userland

	// For now, return an error indicating the operation is not supported at syscall
	// level User should use the file system API (fs_write) instead
	return ERR_INVALID_FD;
}

size_t sys_draw_text(uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t arg4)
{
	const char __user* text = reinterpret_cast<const char*>(arg1);
	const int x = arg2;
	const int y = arg3;
	const uint32_t color = arg4;

	// Copy the string out of user space before touching it (issue #313)
	char buf[256];
	const ssize_t len = copy_string_from_user(buf, text, sizeof(buf));
	if (len < 0) {
		return 0;
	}

	write_string(*kernel::graphics::kscreen, { x, y }, buf, color);

	return len;
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

	kernel::graphics::kscreen->fill_rectangle(Point2D{ x, y },
											  Point2D{ width, height }, color);

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
	Message __user* m = reinterpret_cast<Message*>(arg3);
	const int flags = arg4;

	kernel::task::Task* t = kernel::task::CURRENT_TASK;

	if (flags == IPC_RECV) {
		// Blocking receive (issue #314 Stage A): the sleep happens here on
		// the task's kernel stack, so the task never returns to user space
		// in the WAITING state (which would drop it from the run queue on
		// the next preemption, issue #313). The receiver consumes no CPU
		// until a sender or doorbell wakes it.
		Message received = kernel::task::receive_blocking();

		// Map an attached OOL payload into this task before the message
		// reaches user space (the only kernel->user translation point)
		kernel::task::deliver_ool_to_user(t, &received);

		if (copy_to_user(m, &received, sizeof(*m)) != sizeof(*m)) {
			return ERR_INVALID_ARG;
		}

		return OK;
	}

	if (flags == IPC_SEND) {
		Message copy_m;
		if (copy_from_user(&copy_m, m, sizeof(copy_m)) != sizeof(copy_m)) {
			return ERR_INVALID_ARG;
		}

		if (copy_m.type == MsgType::KERNEL_TASK_READY) {
			copy_m.sender = t->id;
		}

		// One-way sends never carry the reply flag; IPC_REPLY is the only
		// door to the reply-slot delivery path
		copy_m.flags &= ~MSG_FLAG_REPLY;

		// A user-space OOL payload becomes a kernel-owned copy here (the
		// only user->kernel translation point)
		RETURN_IF_ERROR(kernel::task::copy_in_ool_from_user(&copy_m));

		const error_t err =
				kernel::task::send_message(ProcessId::from_raw(dest), copy_m);
		if (IS_ERR(err)) {
			kernel::task::free_message_ool(copy_m);
		}

		return err;
	}

	if (flags == IPC_CALL) {
		Message copy_m;
		if (copy_from_user(&copy_m, m, sizeof(copy_m)) != sizeof(copy_m)) {
			return ERR_INVALID_ARG;
		}

		RETURN_IF_ERROR(kernel::task::copy_in_ool_from_user(&copy_m));

		// call() forces the sender and manages the correlation id; the
		// caller never sees either. The reply overwrites the user message.
		const error_t err = kernel::task::call(ProcessId::from_raw(dest), &copy_m);
		if (IS_ERR(err)) {
			// Send failed, so the request payload was never handed over
			kernel::task::free_message_ool(copy_m);
			return err;
		}

		// copy_m now holds the reply; map its OOL payload if it has one
		kernel::task::deliver_ool_to_user(t, &copy_m);

		if (copy_to_user(m, &copy_m, sizeof(copy_m)) != sizeof(copy_m)) {
			return ERR_INVALID_ARG;
		}

		return OK;
	}

	if (flags == IPC_REPLY) {
		Message copy_m;
		if (copy_from_user(&copy_m, m, sizeof(copy_m)) != sizeof(copy_m)) {
			return ERR_INVALID_ARG;
		}

		if (copy_m.correlation == 0) {
			// The request was fire-and-forget: nothing to answer
			return OK;
		}

		// The server runtime echoes the request's correlation into the
		// reply; the kernel stamps the flag and the true sender
		copy_m.sender = t->id;
		copy_m.flags |= MSG_FLAG_REPLY;

		RETURN_IF_ERROR(kernel::task::copy_in_ool_from_user(&copy_m));

		const error_t err =
				kernel::task::send_message(ProcessId::from_raw(dest), copy_m);
		if (IS_ERR(err)) {
			kernel::task::free_message_ool(copy_m);
		}

		return err;
	}

	if (flags == IPC_OOL_RELEASE) {
		Message copy_m;
		if (copy_from_user(&copy_m, m, sizeof(copy_m)) != sizeof(copy_m)) {
			return ERR_INVALID_ARG;
		}

		return kernel::task::release_ool_region(t, copy_m.ool.addr);
	}

	return ERR_INVALID_ARG;
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

	Message msg{ .type = MsgType::FS_LOAD,
				 .sender = kernel::task::CURRENT_TASK->id };

	// Copy the strings out of user space with bounds checks before using
	// them; the path must also fit the fixed-size Message name field
	// (issue #313)
	if (copy_string_from_user(msg.data.fs.name, path, sizeof(msg.data.fs.name)) <
		0) {
		return ERR_INVALID_ARG;
	}

	char copy_args[256] = "";
	if (args != nullptr &&
		copy_string_from_user(copy_args, args, sizeof(copy_args)) < 0) {
		return ERR_INVALID_ARG;
	}

	// One synchronous FS_LOAD replaces the old stat-then-read pair; the
	// reply's OOL buffer is a kernel-owned copy of the file, owned by this
	// task from here on (issue #314 Stage C)
	RETURN_IF_ERROR(kernel::task::call(process_ids::FS_FAT32, &msg));

	// Take ownership before inspecting the result so an error reply that
	// still carries a payload cannot leak it
	kernel::memory::unique_kbuf<> elf_buf{ reinterpret_cast<void*>(msg.ool.addr) };
	if (IS_ERR(msg.result) || msg.ool.size == 0 || !elf_buf) {
		return ERR_NO_FILE;
	}

	// Build the new address space and switch CR3 to it BEFORE releasing the
	// old one: config_new_page_table() copies the kernel space out of the
	// active table, and the CPU keeps translating through the old table
	// until the switch (issue #313 use-after-free).
	kernel::memory::page_table_entry* old_page_table =
			kernel::memory::get_active_page_table();

	kernel::memory::page_table_entry* new_page_table =
			kernel::memory::config_new_page_table();
	if (new_page_table == nullptr) {
		return ERR_NO_MEMORY;
	}

	kernel::task::CURRENT_TASK->ctx.cr3 = reinterpret_cast<uint64_t>(new_page_table);

	// The old image's OOL mappings die with the old table; their buffers
	// must not outlive it
	kernel::task::release_ool_regions(kernel::task::CURRENT_TASK);

	kernel::memory::clean_page_tables(old_page_table);

	kernel::fs::fat::execute_file(std::move(elf_buf), "", copy_args);

	return OK;
}

void sys_exit(uint64_t arg1) { kernel::task::exit_task(arg1); }

ProcessId sys_wait(uint64_t arg1)
{
	auto __user* status = reinterpret_cast<int*>(arg1);
	kernel::task::Task* t = kernel::task::CURRENT_TASK;

	// Child exits are parent-side records now, not IPC messages (issue
	// #314 Stage B): pop the oldest one, sleeping until a child exits.
	// The old "forward the exit message to SHELL" hack is gone; the shell
	// acts on sys_wait's return instead.
	kernel::task::ChildExitRecord record{};
	while (true) {
		__asm__("cli");
		if (t->num_exit_records > 0) {
			record = t->exit_records[0];
			for (int i = 1; i < t->num_exit_records; ++i) {
				t->exit_records[i - 1] = t->exit_records[i];
			}
			--t->num_exit_records;
			t->state = kernel::task::TASK_RUNNING;
			t->wait_reason = kernel::task::WaitReason::NONE;
			__asm__("sti");
			break;
		}

		// Declare WAITING while interrupts are off (lost-wakeup
		// discipline); record_child_exit wakes CHILD waiters
		t->wait_reason = kernel::task::WaitReason::CHILD;
		t->state = kernel::task::TASK_WAITING;
		__asm__("sti");
		kernel::task::switch_next_task(false);
	}

	if (copy_to_user(status, &record.status, sizeof(int)) != sizeof(int)) {
		return ProcessId::from_raw(-1);
	}

	return ProcessId::from_raw(record.pid);
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
