#include "task.hpp"
#include "../memory/page_operations.h"
#include "../memory/segment.hpp"

Task::Task(int id, uint64_t task_addr, bool is_runnning, int priority)
	: id_{ id },
	  priority_{ priority },
	  is_running_{ is_runnning },
	  stack_{ 4096 },
	  context_{ 0 }
{
	const size_t stack_size = 4096 / sizeof(stack_[0]);
	stack_.resize(stack_size);
	const uint64_t stack_end = reinterpret_cast<uint64_t>(&stack_[stack_size]);

	context_.rsp = (stack_end & ~0xfLU) - 8;
	context_.cr3 = get_cr3();
	context_.rflags = 0x202;
	context_.rip = task_addr;
	context_.cs = KERNEL_CS;
	context_.ss = KERNEL_SS;
}