#include "task.hpp"
#include "../graphics/terminal.hpp"
#include "../memory/page_operations.h"
#include "../memory/segment.hpp"
#include "../memory/slab.hpp"
#include "../timers/timer.hpp"
#include "../types.hpp"
#include "context_switch.h"

int last_task_id_;
list_t run_queue_list_;
task* CURRENT_TASK = nullptr;

task* add_task(uint64_t task_addr, int priority, bool is_init, bool is_running)
{
	void* addr = kmalloc(sizeof(task), KMALLOC_UNINITIALIZED);

	return new (addr)
			task(last_task_id_++, task_addr, is_running, priority, is_init);
}

task* get_scheduled_task()
{
	if (list_is_empty(&run_queue_list_)) {
		return nullptr;
	}

	task* scheduled_task = LIST_POP_FRONT(&run_queue_list_, task, run_queue_elem);
	CURRENT_TASK = scheduled_task;

	return scheduled_task;
}

void switch_task(const context& current_ctx)
{
	memcpy(&CURRENT_TASK->ctx, &current_ctx, sizeof(context));
	list_push_back(&run_queue_list_, &CURRENT_TASK->run_queue_elem);

	task* scheduled_task = get_scheduled_task();
	if (scheduled_task == nullptr) {
		main_terminal->print("No task to schedule\n");
		return;
	}

	restore_context(&scheduled_task->ctx);
}

void initialize_task_manager()
{
	list_init(&run_queue_list_);

	CURRENT_TASK = new task(last_task_id_, 0, true, 2, false);

	++last_task_id_;

	auto* task_2 = add_task(reinterpret_cast<uint64_t>(&task_a), 2, true, true);

	list_push_back(&run_queue_list_, &task_2->run_queue_elem);

	ktimer->add_switch_task_event(SWITCH_TEXT_MILLISEC);
}

task::task(int id, uint64_t task_addr, bool is_running, int priority, bool is_init)
	: id{ id }, priority{ priority }, is_running{ is_running }, stack{ 0 }, ctx{ 0 }
{
	list_elem_init(&run_queue_elem);

	if (!is_init) {
		return;
	}

	const size_t stack_size = 4096 / sizeof(stack[0]);
	stack.resize(stack_size);
	const uint64_t stack_end = reinterpret_cast<uint64_t>(&stack[stack_size]);

	memset(&ctx, 0, sizeof(ctx));

	ctx.rsp = (stack_end & ~0xfLU) - 8;
	ctx.cr3 = get_cr3();
	ctx.rflags = 0x202;
	ctx.rip = task_addr;
	ctx.cs = KERNEL_CS;
	ctx.ss = KERNEL_SS;

	*reinterpret_cast<uint32_t*>(&ctx.fxsave_area[24]) = 0x1f80;
}

void task_a()
{
	while (true) {
		__asm__("hlt");
	}
}
