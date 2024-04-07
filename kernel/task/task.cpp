#include "task/task.hpp"
#include "file_system/file_descriptor.hpp"
#include "file_system/task.hpp"
#include "graphics/log.hpp"
#include "hardware/usb/task.hpp"
#include "list.hpp"
#include "memory/paging.hpp"
#include "memory/paging_utils.h"
#include "memory/segment.hpp"
#include "task/context_switch.h"
#include "task/ipc.hpp"
#include "timers/timer.hpp"
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <functional>
#include <libs/common/types.hpp>
#include <memory>
#include <queue>
#include <vector>

list_t run_queue;
std::array<task*, MAX_TASKS> tasks;

task* CURRENT_TASK = nullptr;
task* IDLE_TASK = nullptr;

task_t get_available_task_id()
{
	for (task_t i = 0; i < MAX_TASKS; i++) {
		if (tasks[i] == nullptr) {
			return i;
		}
	}

	return -1;
}

task_t get_task_id_by_name(const char* name)
{
	for (task* t : tasks) {
		if (t != nullptr && strcmp(t->name, name) == 0) {
			return t->id;
		}
	}

	return -1;
}

task* create_task(const char* name, uint64_t task_addr, bool is_init)
{
	const task_t task_id = get_available_task_id();
	if (task_id == -1) {
		printk(KERN_ERROR, "failed to allocate task id");
		return nullptr;
	}

	tasks[task_id] = new task(task_id, name, task_addr, TASK_WAITING, is_init);

	return tasks[task_id];
}

error_t task::copy_parent_stack(const context& parent_ctx)
{
	task* parent = tasks[parent_id];
	if (parent == nullptr) {
		return ERR_NO_TASK;
	}

	size_t parent_stack_size = parent->stack.size();
	stack.resize(parent_stack_size);
	memcpy(stack.data(), parent->stack.data(), parent_stack_size * sizeof(uint64_t));

	auto parent_stack_top =
			reinterpret_cast<uint64_t>(&parent->stack[parent_stack_size]);
	auto child_stack_top = reinterpret_cast<uint64_t>(&stack[parent_stack_size]);
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
	task* parent = tasks[parent_id];
	if (parent == nullptr) {
		return ERR_NO_TASK;
	}

	parent->page_table_snapshot =
			reinterpret_cast<page_table_entry*>(parent->ctx.cr3);

	page_table_entry* parent_table =
			clone_page_table(parent->page_table_snapshot, false);
	set_cr3(reinterpret_cast<uint64_t>(parent_table));

	page_table_entry* child_table =
			clone_page_table(parent->page_table_snapshot, false);
	ctx.cr3 = reinterpret_cast<uint64_t>(child_table);

	return OK;
}

task* copy_task(task* parent, context* parent_ctx)
{
	task* child = create_task(parent->name, 0, false);
	if (child == nullptr) {
		return nullptr;
	}
	child->parent_id = parent->id;

	memcpy(&child->ctx, parent_ctx, sizeof(context));

	if (IS_ERR(child->copy_parent_stack(*parent_ctx))) {
		printk(KERN_ERROR, "Failed to copy parent stack : %s", parent->name);
		return nullptr;
	}

	if (IS_ERR(child->copy_parent_page_table())) {
		printk(KERN_ERROR, "Failed to copy parent page table : %s", parent->name);
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

void schedule_task(task_t id)
{
	if (tasks.size() <= id || tasks[id] == nullptr) {
		printk(KERN_ERROR, "schedule_task: task %d is not found", id);
		return;
	}

	tasks[id]->state = TASK_READY;

	if (list_contains(&run_queue, &tasks[id]->run_queue_elem)) {
		return;
	}

	list_push_back(&run_queue, &tasks[id]->run_queue_elem);
}

void switch_task(const context& current_ctx)
{
	if (CURRENT_TASK->state == TASK_EXITED) {
		delete tasks[CURRENT_TASK->id];
		tasks[CURRENT_TASK->id] = nullptr;
	} else {
		memcpy(&CURRENT_TASK->ctx, &current_ctx, sizeof(context));
		if (CURRENT_TASK->state != TASK_WAITING) {
			schedule_task(CURRENT_TASK->id);
		}
	}

	restore_context(&get_scheduled_task()->ctx);
}

void exit_task(int status)
{
	task* t = CURRENT_TASK;

	if (t->has_parent()) {
		message m = { .type = IPC_EXIT_TASK, .sender = t->id };
		m.data.exit_task.status = status;
		send_message(t->parent_id, &m);
	}

	t->state = TASK_EXITED;
	switch_task(t->ctx);
}

[[noreturn]] void process_messages(task* t)
{
	while (true) {
		if (t->messages.empty()) {
			t->state = TASK_WAITING;
			continue;
		}

		t->state = TASK_RUNNING;

		const message m = t->messages.front();
		t->messages.pop();

		if (m.type < 0 || m.type >= NUM_MESSAGE_TYPES) {
			continue;
		}

		t->message_handlers[m.type](m);
	}
}

fd_t allocate_fd(task* t)
{
	for (fd_t i = 0; i < t->fds.size(); i++) {
		if (t->fds[i] == nullptr) {
			return i;
		}
	}

	return NO_FD;
}

void initialize_task()
{
	tasks = std::array<task*, MAX_TASKS>();
	list_init(&run_queue);

	task* main_task = create_task("main", 0, false);
	main_task->state = TASK_RUNNING;
	CURRENT_TASK = main_task;

	IDLE_TASK = create_task("idle", reinterpret_cast<uint64_t>(&task_idle), true);

	auto* usb_task = create_task(
			"usb_handler", reinterpret_cast<uint64_t>(&task_usb_handler), true);
	schedule_task(usb_task->id);

	task* file_system_task = create_task(
			"file_system", reinterpret_cast<uint64_t>(&task_file_system), true);
	schedule_task(file_system_task->id);

	ktimer->add_switch_task_event(200);
}

task::task(int id,
		   const char* task_name,
		   uint64_t task_addr,
		   task_state state,
		   bool is_init)
	: id{ id },
	  parent_id{ -1 },
	  priority{ 2 }, // TODO: Implement priority scheduling
	  state{ state },
	  stack{ std::vector<uint64_t>() },
	  messages{ std::queue<message>() },
	  message_handlers{
		  std::array<std::function<void(const message&)>, NUM_MESSAGE_TYPES>()
	  },
	  fds{ std::array<std::shared_ptr<file_descriptor>, 10>() }
{
	list_elem_init(&run_queue_elem);

	for (auto& handler : message_handlers) {
		handler = [](const message&) {};
	}

	strncpy(name, task_name, sizeof(name) - 1);
	name[sizeof(name) - 1] = '\0';

	for (int i = 0; i < 3; ++i) {
		fds[i].reset();
	}

	if (!is_init) {
		return;
	}

	const size_t stack_size = 4096 * 8 / sizeof(stack[0]);
	stack.resize(stack_size);
	const uint64_t stack_end = reinterpret_cast<uint64_t>(&stack[stack_size]);

	memset(&ctx, 0, sizeof(ctx));

	page_table_entry* pml4 = new_page_table();
	page_table_entry* current_pml4 = reinterpret_cast<page_table_entry*>(get_cr3());
	// copy kernel space mapping
	memcpy(pml4, current_pml4, 256 * sizeof(page_table_entry));

	ctx.cr3 = reinterpret_cast<uint64_t>(pml4);
	ctx.rsp = (stack_end & ~0xfLU) - 8;
	ctx.rflags = 0x202;
	ctx.rip = task_addr;
	ctx.cs = KERNEL_CS;
	ctx.ss = KERNEL_SS;
	*reinterpret_cast<uint32_t*>(&ctx.fxsave_area[24]) = 0x1f80;
}

[[noreturn]] void task_idle()
{
	while (true) {
		__asm__("hlt");
	}
}

extern "C" uint64_t get_current_task_stack()
{
	return CURRENT_TASK->kernel_stack_ptr;
}
