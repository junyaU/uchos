#include "task.hpp"
#include "../graphics/terminal.hpp"
#include "../memory/page_operations.h"
#include "../memory/segment.hpp"
#include "../memory/slab.hpp"
#include "../timers/timer.hpp"
#include "../types.hpp"
#include "context_switch.h"

int last_task_id_;
std::deque<std::unique_ptr<Task>> tasks_;

int add_task(uint64_t task_addr, int priority, bool is_init, bool is_running)
{
	void* addr = kmalloc(sizeof(Task), KMALLOC_UNINITIALIZED);
	Task* task =
			new (addr) Task(last_task_id_, task_addr, is_running, priority, is_init);

	tasks_.emplace_back(task);

	++last_task_id_;

	return task->ID();
}

void switch_task(const context& current_ctx)
{
	memcpy(&tasks_.front().get()->TaskContext(), &current_ctx, sizeof(context));

	tasks_.push_back(std::move(tasks_.front()));
	tasks_.pop_front();

	restore_context(&tasks_.front().get()->TaskContext());
}

void initialize_task_manager()
{
	tasks_ = std::deque<std::unique_ptr<Task>>();
	tasks_.emplace_back(new Task(last_task_id_, 0, true, 2, false));
	++last_task_id_;

	add_task(reinterpret_cast<uint64_t>(&task_a), 2, true, true);

	ktimer->add_switch_task_event(SWITCH_TEXT_MILLISEC);
}

Task::Task(int id, uint64_t task_addr, bool is_runnning, int priority, bool is_init)
	: id_{ id },
	  priority_{ priority },
	  is_running_{ is_runnning },
	  stack_{ 0 },
	  context_{ 0 }
{
	if (!is_init) {
		return;
	}

	const size_t stack_size = 4096 / sizeof(stack_[0]);
	stack_.resize(stack_size);
	const uint64_t stack_end = reinterpret_cast<uint64_t>(&stack_[stack_size]);

	memset(&context_, 0, sizeof(context_));

	context_.rsp = (stack_end & ~0xfLU) - 8;
	context_.cr3 = get_cr3();
	context_.rflags = 0x202;
	context_.rip = task_addr;
	context_.cs = KERNEL_CS;
	context_.ss = KERNEL_SS;

	*reinterpret_cast<uint32_t*>(&context_.fxsave_area[24]) = 0x1f80;
}

void task_a()
{
	while (true) {
		__asm__("hlt");
	}
}
