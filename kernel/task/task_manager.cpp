#include "task_manager.hpp"

#include "../point2d.hpp"
#include "context.hpp"
#include "graphics/screen.hpp"
#include "graphics/system_logger.hpp"
#include "memory/memory.h"
#include "memory/segment.hpp"
#include "system_event.hpp"
#include "task.h"
#include "timers/timer.hpp"

#include <cstdio>
#include <vector>

Task::Task(int id, uint64_t task_addr) : id_{ id }, stack_{ 4096 }, context_{ 0 }
{
	const size_t stack_size = 4096 / sizeof(stack_[0]);
	stack_.resize(stack_size);
	uint64_t stack_end = reinterpret_cast<uint64_t>(&stack_[stack_size]);

	context_.rsp = (stack_end & ~0xflu) - 8;
	context_.cr3 = GetCR3();
	context_.rflags = 0x202;
	context_.rip = task_addr;
	context_.cs = kKernelCS;
	context_.ss = kKernelSS;
}

TaskManager::TaskManager() : last_task_id_{ 0 }, current_task_index_{ 0 }
{
	tasks_.emplace_back(new Task(last_task_id_, 0));
	++last_task_id_;
}

int TaskManager::AddTask(uint64_t task_addr)
{
	int current_task_id = last_task_id_;
	tasks_.emplace_back(new Task(current_task_id, task_addr));

	++last_task_id_;

	return current_task_id;
}

void TaskManager::SwitchTask()
{
	if (tasks_.size() <= 1) {
		return;
	}

	Task& current_task = *tasks_[current_task_index_];

	if (current_task_index_ == tasks_.size() - 1) {
		current_task_index_ = 0;
	} else {
		++current_task_index_;
	}

	Task& next_task = *tasks_[current_task_index_];

	ExecuteContextSwitch(&next_task.TaskContext(), &current_task.TaskContext());
}

// test task
void Task2()
{
	int i = 0;
	while (true) {
		char count[14];

		sprintf(count, "%d", i);

		Point2D draw_area = { static_cast<int>(300),
							  static_cast<int>(screen->Height() -
											   screen->Height() * 0.08) };

		screen->FillRectangle(draw_area,
							  { bitmap_font->Width() * 8, bitmap_font->Height() },
							  screen->TaskbarColor().GetCode());

		screen->DrawString(draw_area, count, 0xffffff);

		i++;
	}
}

void Task3()
{
	while (true) {
		__asm__("hlt");
	}
}

TaskManager* task_manager;

void InitializeTaskManager()
{
	task_manager = new TaskManager();

	task_manager->AddTask(reinterpret_cast<uint64_t>(Task2));
	task_manager->AddTask(reinterpret_cast<uint64_t>(Task3));

	timer->AddSwitchTaskEvent(kSwitchTextMillisec);
}