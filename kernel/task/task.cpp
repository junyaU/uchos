#include "task/task.hpp"
#include "file_system/fat.hpp"
#include "file_system/path.hpp"
#include "graphics/log.hpp"
#include "hardware/virtio/blk.hpp"
#include "interrupt/vector.hpp"
#include "list.hpp"
#include "memory/page.hpp"
#include "memory/paging.hpp"
#include "memory/paging_utils.h"
#include "memory/segment.hpp"
#include "task/builtin.hpp"
#include "task/context_switch.h"
#include "task/ipc.hpp"
#include "tests/framework.hpp"
#include "tests/test_cases/task_test.hpp"
#include "timers/timer.hpp"
#include <array>
#include <cstdint>
#include <cstring>
#include <libs/common/message.hpp>
#include <libs/common/types.hpp>
#include <libs/common/process_id.hpp>
#include <queue>

list_t run_queue;
std::array<task*, MAX_TASKS> tasks;

task* CURRENT_TASK = nullptr;
task* IDLE_TASK = nullptr;

const initial_task_info initial_tasks[] = {
	{ "main", 0, false, true },
	{ "idle", reinterpret_cast<uint64_t>(&task_idle), true, true },
	{ "usb_handler", reinterpret_cast<uint64_t>(&task_usb_handler), true, true },
	{ "virtio", reinterpret_cast<uint64_t>(&virtio_blk_task), true, false },
	{ "fat32", reinterpret_cast<uint64_t>(&file_system::fat32_task), true, true },
	{ "shell", reinterpret_cast<uint64_t>(&task_shell), true, false },
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
	for (task* t : tasks) {
		if (t != nullptr && strcmp(t->name, name) == 0) {
			return t->id;
		}
	}

	return ProcessId::from_raw(-1);
}

task* get_task(ProcessId id)
{
	pid_t raw_id = id.raw();
	if (raw_id < 0 || raw_id >= MAX_TASKS) {
		return nullptr;
	}

	return tasks[raw_id];
}

task* create_task(const char* name,
				  uint64_t task_addr,
				  bool setup_context,
				  bool is_init)
{
	const ProcessId task_id = get_available_task_id();
	if (task_id.raw() == -1) {
		LOG_ERROR("failed to allocate task id");
		return nullptr;
	}

	tasks[task_id.raw()] =
			new task(task_id.raw(), name, task_addr, TASK_WAITING, setup_context, is_init);

	return tasks[task_id.raw()];
}

error_t task::copy_parent_stack(const context& parent_ctx)
{
	task* parent = tasks[parent_id.raw()];
	if (parent == nullptr) {
		return ERR_NO_TASK;
	}

	stack_size = parent->stack_size;

	stack = static_cast<uint64_t*>(kmalloc(stack_size, kernel::memory::KMALLOC_ZEROED, kernel::memory::PAGE_SIZE));
	if (stack == nullptr) {
		LOG_ERROR("Failed to allocate stack for child task");
		return ERR_NO_MEMORY;
	}

	memcpy(stack, parent->stack, stack_size);

	auto parent_stack_top = reinterpret_cast<uint64_t>(parent->stack) + stack_size;
	auto child_stack_top = reinterpret_cast<uint64_t>(stack) + stack_size;
	uint64_t rsp_offset = parent_stack_top - parent_ctx.rsp;
	uint64_t rbp_offset = parent_stack_top - parent_ctx.rbp;

	ctx.rsp = child_stack_top - rsp_offset;
	ctx.rbp = child_stack_top - rbp_offset;

	uint64_t kernel_sp_offset = parent_stack_top - parent->kernel_stack_ptr;
	kernel_stack_ptr = child_stack_top - kernel_sp_offset;

	return OK;
}

error_t task::copy_parent_page_table()
{
	task* parent = tasks[parent_id.raw()];
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

void task::add_msg_handler(msg_t type, message_handler_t handler)
{
	if (type == msg_t::NO_TASK || type >= msg_t::MAX_MESSAGE_TYPE) {
		return;
	}
	message_handlers[static_cast<int32_t>(type)] = handler;
}

task* copy_task(task* parent, context* parent_ctx)
{
	task* child = create_task(parent->name, 0, false, true);
	if (child == nullptr) {
		return nullptr;
	}
	child->parent_id = parent->id;

	memcpy(&child->ctx, parent_ctx, sizeof(context));

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

task* get_scheduled_task()
{
	if (list_is_empty(&run_queue)) {
		CURRENT_TASK = IDLE_TASK;
		return IDLE_TASK;
	}

	task* scheduled_task = LIST_POP_FRONT(&run_queue, task, run_queue_elem);
	scheduled_task->state = TASK_RUNNING;
	CURRENT_TASK = scheduled_task;

	return scheduled_task;
}

void schedule_task(ProcessId id)
{
	pid_t raw_id = id.raw();
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

void switch_task(const context& current_ctx)
{
	if (CURRENT_TASK->state == TASK_EXITED) {
		delete tasks[CURRENT_TASK->id.raw()];
		tasks[CURRENT_TASK->id.raw()] = nullptr;
	} else {
		memcpy(&CURRENT_TASK->ctx, &current_ctx, sizeof(context));
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

	asm("int %0" : : "i"(interrupt_vector::SWITCH_TASK));
}

void exit_task(int status)
{
	task* t = CURRENT_TASK;

	if (t->has_parent()) {
		message m = { .type = msg_t::IPC_EXIT_TASK, .sender = t->id };
		m.data.exit_task.status = status;
		send_message(t->parent_id, m);
	}

	t->state = TASK_EXITED;
	switch_task(t->ctx);
}

[[noreturn]] void process_messages(task* t)
{
	while (true) {
		if (t->messages.empty()) {
			switch_next_task(true);
			continue;
		}

		t->state = TASK_RUNNING;

		const message m = t->messages.front();
		t->messages.pop();

		if (m.type == msg_t::NO_TASK || m.type >= msg_t::MAX_MESSAGE_TYPE) {
			continue;
		}

		t->message_handlers[static_cast<int32_t>(m.type)](m);
	}
}

void initialize_task()
{
	tasks = std::array<task*, MAX_TASKS>();
	list_init(&run_queue);

	for (const auto& t_info : initial_tasks) {
		task* new_task = create_task(t_info.name, t_info.addr, t_info.setup_context,
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

task::task(int raw_id,
		   const char* task_name,
		   uint64_t task_addr,
		   task_state state,
		   bool setup_context,
		   bool is_initilized)
	: id{ ProcessId::from_raw(raw_id) },
	  parent_id{ ProcessId::from_raw(-1) },
	  priority{ 2 }, // TODO: Implement priority scheduling
	  is_initilized{ is_initilized },
	  state{ state },
	  fs_path({ nullptr, nullptr, nullptr }),
	  stack{ nullptr },
	  messages{ std::queue<message>() },
	  message_handlers({ std::array<message_handler_t, total_message_types>() })
{
	list_elem_init(&run_queue_elem);

	for (auto& handler : message_handlers) {
		handler = [](const message&) {};
	}

	strncpy(name, task_name, sizeof(name) - 1);
	name[sizeof(name) - 1] = '\0';

	if (!setup_context) {
		return;
	}

	stack_size = kernel::memory::PAGE_SIZE * 8;
	stack = static_cast<uint64_t*>(kmalloc(stack_size, kernel::memory::KMALLOC_ZEROED, kernel::memory::PAGE_SIZE));
	if (stack == nullptr) {
		LOG_ERROR("Failed to allocate stack for task %s", name);
		return;
	}

	const uint64_t stack_end = reinterpret_cast<uint64_t>(stack) + stack_size;

	kernel::memory::page_table_entry* page_table = kernel::memory::new_page_table();
	kernel::memory::copy_kernel_space(page_table);

	ctx = {};
	ctx.cr3 = reinterpret_cast<uint64_t>(page_table);
	ctx.rsp = (stack_end & ~0xfLU) - 8;
	ctx.rflags = 0x202;
	ctx.rip = task_addr;
	ctx.cs = KERNEL_CS;
	ctx.ss = KERNEL_SS;
	*reinterpret_cast<uint32_t*>(&ctx.fxsave_area[24]) = 0x1f80;
}

message wait_for_message(msg_t type)
{
	task* t = CURRENT_TASK;

	while (true) {
		if (t->messages.empty()) {
			switch_next_task(true);
			continue;
		}

		t->state = TASK_RUNNING;

		message m = t->messages.front();
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

extern "C" uint64_t get_current_task_stack()
{
	return CURRENT_TASK->kernel_stack_ptr;
}
