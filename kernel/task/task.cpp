#include "task.hpp"
#include "../graphics/terminal.hpp"
#include "../memory/page_operations.h"
#include "../memory/segment.hpp"
#include "../timers/timer.hpp"

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
