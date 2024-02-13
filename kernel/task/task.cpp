#include "task.hpp"
#include "../graphics/terminal.hpp"
#include "../hardware/usb/task.hpp"
#include "../list.hpp"
#include "../memory/page_operations.h"
#include "../memory/paging.hpp"
#include "../memory/segment.hpp"
#include "../timers/timer.hpp"
#include "../types.hpp"
#include "context_switch.h"
#include <array>
#include <cstdint>
#include <cstring>
#include <functional>
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

task* create_task(const char* name, uint64_t task_addr, int priority, bool is_init)
{
	const task_t task_id = get_available_task_id();
	if (task_id == -1) {
		printk(KERN_ERROR, "failed to allocate task id");
		return nullptr;
	}

	tasks[task_id] =
			new task(task_id, name, task_addr, TASK_WAITING, priority, is_init);

	return tasks[task_id];
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
	if (tasks[id] == nullptr) {
		printk(KERN_ERROR, "task %d is not found", id);
		return;
	}

	if (list_contains(&run_queue, &tasks[id]->run_queue_elem)) {
		return;
	}

	tasks[id]->state = TASK_READY;
	list_push_back(&run_queue, &tasks[id]->run_queue_elem);
}

void switch_task(const context& current_ctx)
{
	memcpy(&CURRENT_TASK->ctx, &current_ctx, sizeof(context));

	if (CURRENT_TASK->state != TASK_WAITING) {
		schedule_task(CURRENT_TASK->id);
	}

	restore_context(&get_scheduled_task()->ctx);
}

void process_messages(task* t)
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

void initialize_task()
{
	tasks = std::array<task*, MAX_TASKS>();
	list_init(&run_queue);

	task* main_task = create_task("main", 0, 2, false);
	main_task->state = TASK_RUNNING;
	CURRENT_TASK = main_task;

	IDLE_TASK = create_task("idle", reinterpret_cast<uint64_t>(&task_idle), 2, true);

	auto* terminal_task = create_task(
			"terminal", reinterpret_cast<uint64_t>(&task_terminal), 2, true);
	schedule_task(terminal_task->id);

	auto* usb_task = create_task(
			"usb_handler", reinterpret_cast<uint64_t>(&task_usb_handler), 2, true);
	schedule_task(usb_task->id);

	ktimer->add_switch_task_event(200);
}

task::task(int id,
		   const char* task_name,
		   uint64_t task_addr,
		   task_state state,
		   int priority,
		   bool is_init)
	: id{ id },
	  priority{ priority },
	  state{ state },
	  stack{ std::vector<uint64_t>() },
	  messages{ std::queue<message>() },
	  message_handlers{
		  std::array<std::function<void(const message&)>, NUM_MESSAGE_TYPES>()
	  }
{
	list_elem_init(&run_queue_elem);

	for (auto& handler : message_handlers) {
		handler = [](const message&) {};
	}

	strncpy(name, task_name, sizeof(name) - 1);
	name[sizeof(name) - 1] = '\0';

	if (!is_init) {
		return;
	}

	const size_t stack_size = 4096 / sizeof(stack[0]);
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

void task_idle()
{
	while (true) {
		__asm__("hlt");
	}
}
