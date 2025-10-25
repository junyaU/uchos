#include "task/task.hpp"
#include <array>
#include <cstdint>
#include <cstring>
#include <libs/common/message.hpp>
#include <libs/common/process_id.hpp>
#include <libs/common/types.hpp>
#include <queue>
#include "fs/fat/fat.hpp"
#include "fs/path.hpp"
#include "graphics/log.hpp"
#include "hardware/virtio/blk.hpp"
#include "hardware/virtio/net.hpp"
#include "interrupt/vector.hpp"
#include "list.hpp"
#include "memory/page.hpp"
#include "memory/paging.hpp"
#include "memory/paging_utils.h"
#include "memory/segment.hpp"
#include "memory/slab.hpp"
#include "task/builtin.hpp"
#include "task/context.hpp"
#include "task/context_switch.h"
#include "task/ipc.hpp"
#include "tests/framework.hpp"
#include "tests/test_cases/task_test.hpp"
#include "timers/timer.hpp"

namespace kernel::task
{

list_t run_queue;
std::array<Task*, MAX_TASKS> tasks;

Task* CURRENT_TASK = nullptr;
Task* IDLE_TASK = nullptr;

const InitialTaskInfo INITIAL_TASKS[] = {
	{ "main", 0, false, true },
	{ "idle", reinterpret_cast<uint64_t>(&kernel::task::idle_service), true, true },
	{ "usb_handler", reinterpret_cast<uint64_t>(&kernel::task::usb_handler_service),
	  true, true },
	{ "virtio_blk",
	  reinterpret_cast<uint64_t>(&kernel::hw::virtio::virtio_blk_service), true,
	  false },
	{ "fat32", reinterpret_cast<uint64_t>(&kernel::fs::fat32_service), true, true },
	{ "shell", reinterpret_cast<uint64_t>(&kernel::task::shell_service), true,
	  false },
	{ "virtio_net",
	  reinterpret_cast<uint64_t>(&kernel::hw::virtio::virtio_net_service), true,
	  false },
};

ProcessId get_available_task_id()
{
	for (pid_t i = 0; i < MAX_TASKS; i++) {
		if (tasks[i] == nullptr) {
			return ProcessId::from_raw(i);
		}
	}

	return ProcessId::from_raw(-1);
}

ProcessId get_task_id_by_name(const char* name)
{
	for (Task* t : tasks) {
		if (t != nullptr && strcmp(t->name, name) == 0) {
			return t->id;
		}
	}

	return ProcessId::from_raw(-1);
}

Task* get_task(ProcessId id)
{
	const pid_t raw_id = id.raw();
	if (raw_id < 0 || raw_id >= MAX_TASKS) {
		return nullptr;
	}

	return tasks[raw_id];
}

Task* create_task(const char* name,
				  uint64_t task_addr,
				  bool setup_context,
				  bool is_init)
{
	const ProcessId task_id = get_available_task_id();
	if (task_id.raw() == -1) {
		LOG_ERROR("failed to allocate task id");
		return nullptr;
	}

	tasks[task_id.raw()] = new Task(task_id.raw(), name, task_addr, TASK_WAITING,
									setup_context, is_init);

	return tasks[task_id.raw()];
}

error_t Task::copy_parent_stack(const Context& parent_ctx)
{
	Task* parent = tasks[parent_id.raw()];
	if (parent == nullptr) {
		return ERR_NO_TASK;
	}

	stack_size = parent->stack_size;

	void* stack_ptr;
	ALLOC_OR_RETURN_ERROR(stack_ptr, stack_size, kernel::memory::ALLOC_ZEROED);
	stack = static_cast<uint64_t*>(stack_ptr);

	memcpy(stack, parent->stack, stack_size);

	auto parent_stack_top = reinterpret_cast<uint64_t>(parent->stack) + stack_size;
	auto child_stack_top = reinterpret_cast<uint64_t>(stack) + stack_size;
	const uint64_t rsp_offset = parent_stack_top - parent_ctx.rsp;
	const uint64_t rbp_offset = parent_stack_top - parent_ctx.rbp;

	ctx.rsp = child_stack_top - rsp_offset;
	ctx.rbp = child_stack_top - rbp_offset;

	const uint64_t kernel_sp_offset = parent_stack_top - parent->kernel_stack_ptr;
	kernel_stack_ptr = child_stack_top - kernel_sp_offset;

	return OK;
}

error_t Task::copy_parent_page_table()
{
	Task* parent = tasks[parent_id.raw()];
	if (parent == nullptr) {
		return ERR_NO_TASK;
	}

	parent->page_table_snapshot =
			reinterpret_cast<kernel::memory::page_table_entry*>(parent->ctx.cr3);

	kernel::memory::page_table_entry* parent_table =
			kernel::memory::clone_page_table(parent->page_table_snapshot, false);
	set_cr3(reinterpret_cast<uint64_t>(parent_table));

	kernel::memory::page_table_entry* child_table =
			kernel::memory::clone_page_table(parent->page_table_snapshot, false);
	ctx.cr3 = reinterpret_cast<uint64_t>(child_table);

	return OK;
}

void Task::add_msg_handler(MsgType type, message_handler_t handler)
{
	if (type == MsgType::NO_TASK || type >= MsgType::MAX_MESSAGE_TYPE) {
		return;
	}
	message_handlers[static_cast<int32_t>(type)] = handler;
}

Task* copy_task(Task* parent, Context* parent_ctx)
{
	Task* child = create_task(parent->name, 0, false, true);
	if (child == nullptr) {
		return nullptr;
	}
	child->parent_id = parent->id;

	memcpy(&child->ctx, parent_ctx, sizeof(Context));

	// Copy parent's file descriptor table
	if (IS_ERR(kernel::fs::copy_fd_table(child->fd_table.data(),
										 parent->fd_table.data(),
										 MAX_FDS_PER_PROCESS, child->id))) {
		LOG_ERROR("Failed to copy file descriptor table : %s", parent->name);
		return nullptr;
	}

	if (IS_ERR(child->copy_parent_stack(*parent_ctx))) {
		LOG_ERROR("Failed to copy parent stack : %s", parent->name);
		return nullptr;
	}

	if (IS_ERR(child->copy_parent_page_table())) {
		LOG_ERROR("Failed to copy parent page table : %s", parent->name);
		return nullptr;
	}

	return child;
}

Task* get_scheduled_task()
{
	if (list_is_empty(&run_queue)) {
		CURRENT_TASK = IDLE_TASK;
		return IDLE_TASK;
	}

	Task* scheduled_task = LIST_POP_FRONT(&run_queue, Task, run_queue_elem);
	scheduled_task->state = TASK_RUNNING;
	CURRENT_TASK = scheduled_task;

	return scheduled_task;
}

void schedule_task(ProcessId id)
{
	const pid_t raw_id = id.raw();
	if (tasks.size() <= raw_id || tasks[raw_id] == nullptr) {
		LOG_ERROR("schedule_task: task %d is not found", raw_id);
		return;
	}

	tasks[raw_id]->state = TASK_READY;

	if (list_contains(&run_queue, &tasks[raw_id]->run_queue_elem)) {
		return;
	}

	list_push_back(&run_queue, &tasks[raw_id]->run_queue_elem);
}

void switch_task(const Context& current_ctx)
{
	if (CURRENT_TASK->state == TASK_EXITED) {
		delete tasks[CURRENT_TASK->id.raw()];
		tasks[CURRENT_TASK->id.raw()] = nullptr;
	} else {
		memcpy(&CURRENT_TASK->ctx, &current_ctx, sizeof(Context));
		if (CURRENT_TASK->state != TASK_WAITING) {
			schedule_task(CURRENT_TASK->id);
		}
	}

	restore_context(&get_scheduled_task()->ctx);
}

void switch_next_task(bool sleep_current_task)
{
	if (sleep_current_task) {
		CURRENT_TASK->state = TASK_WAITING;
	}

	asm("int %0" : : "i"(kernel::interrupt::InterruptVector::SWITCH_TASK));
}

void exit_task(int status)
{
	Task* t = CURRENT_TASK;

	// Release all file descriptors before exiting
	kernel::fs::release_all_process_fds(t->fd_table.data(), MAX_FDS_PER_PROCESS);

	if (t->has_parent()) {
		Message m = { .type = MsgType::IPC_EXIT_TASK, .sender = t->id };
		m.data.exit_task.status = status;
		send_message(t->parent_id, m);
	}

	t->state = TASK_EXITED;
	switch_task(t->ctx);
}

[[noreturn]] void process_messages(Task* t)
{
	while (true) {
		if (t->messages.empty()) {
			switch_next_task(true);
			continue;
		}

		t->state = TASK_RUNNING;

		const Message m = t->messages.front();
		t->messages.pop();

		if (m.type == MsgType::NO_TASK || m.type >= MsgType::MAX_MESSAGE_TYPE) {
			continue;
		}

		t->message_handlers[static_cast<int32_t>(m.type)](m);
	}
}

void initialize()
{
	tasks = std::array<Task*, MAX_TASKS>();
	list_init(&run_queue);

	for (const auto& t_info : INITIAL_TASKS) {
		Task* new_task = create_task(t_info.name, t_info.addr, t_info.setup_context,
									 t_info.is_initilized);
		if (new_task != nullptr) {
			schedule_task(new_task->id);
		}
	}

	CURRENT_TASK = tasks[0];
	IDLE_TASK = tasks[1];

	CURRENT_TASK->state = TASK_RUNNING;

	kernel::timers::ktimer->add_switch_task_event(200);

	run_test_suite(register_task_tests);
}

Task::Task(int raw_id,
		   const char* task_name,
		   uint64_t task_addr,
		   TaskState state,
		   bool setup_context,
		   bool is_initilized)
	: id{ ProcessId::from_raw(raw_id) },
	  parent_id{ ProcessId::from_raw(-1) },
	  priority{ 2 }, // TODO: Implement priority scheduling
	  is_initilized{ is_initilized },
	  state{ state },
	  fs_path({ nullptr, nullptr, nullptr }),
	  stack{ nullptr },
	  messages{ std::queue<Message>() },
	  message_handlers({ std::array<message_handler_t, TOTAL_MESSAGE_TYPES>() }),
	  fd_table()
{
	list_elem_init(&run_queue_elem);

	for (auto& handler : message_handlers) {
		handler = [](const Message&) {};
	}

	// Initialize file descriptor table
	kernel::fs::init_process_fd_table(fd_table.data(), MAX_FDS_PER_PROCESS);

	strncpy(name, task_name, sizeof(name) - 1);
	name[sizeof(name) - 1] = '\0';

	if (!setup_context) {
		return;
	}

	stack_size = kernel::memory::PAGE_SIZE * 8;
	void* stack_ptr;
	ALLOC_OR_RETURN(stack_ptr, stack_size, kernel::memory::ALLOC_ZEROED);
	stack = static_cast<uint64_t*>(stack_ptr);

	const uint64_t stack_end = reinterpret_cast<uint64_t>(stack) + stack_size;

	kernel::memory::page_table_entry* page_table = kernel::memory::new_page_table();
	kernel::memory::copy_kernel_space(page_table);

	ctx = {};
	ctx.cr3 = reinterpret_cast<uint64_t>(page_table);
	ctx.rsp = (stack_end & ~0xfLU) - 8;
	ctx.rflags = 0x202;
	ctx.rip = task_addr;
	ctx.cs = kernel::memory::KERNEL_CS;
	ctx.ss = kernel::memory::KERNEL_SS;
	*reinterpret_cast<uint32_t*>(&ctx.fxsave_area[24]) = 0x1f80;
}

Message wait_for_message(MsgType type)
{
	Task* t = CURRENT_TASK;

	while (true) {
		if (t->messages.empty()) {
			switch_next_task(true);
			continue;
		}

		t->state = TASK_RUNNING;

		Message m = t->messages.front();
		t->messages.pop();

		if (m.type != type) {
			__asm__("cli");
			t->messages.push(m);
			__asm__("sti");
			continue;
		}

		return m;
	}
}

} // namespace kernel::task

extern "C" uint64_t get_current_task_stack()
{
	return kernel::task::CURRENT_TASK->kernel_stack_ptr;
}
