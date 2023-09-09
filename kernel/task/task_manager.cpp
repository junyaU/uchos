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

TaskManager::TaskManager() : last_task_id_{ 0 }
{
	running_tasks_.push_back(new Task(last_task_id_, 0));
	++last_task_id_;
}

int TaskManager::AddTask(uint64_t task_addr, bool is_running)
{
	Task* task = new Task(last_task_id_, task_addr);

	if (is_running) {
		running_tasks_.push_back(task);
	} else {
		wait_tasks_.emplace_back(task);
	}

	++last_task_id_;

	return task->ID();
}

void TaskManager::SwitchTask(bool current_sleep)
{
	if (running_tasks_.size() <= 1) {
		return;
	}

	Task& current_task = *running_tasks_.front();
	running_tasks_.pop_front();

	if (!current_sleep) {
		running_tasks_.push_back(&current_task);
	} else {
		wait_tasks_.emplace_back(&current_task);
	}

	Task& next_task = *running_tasks_.front();

	ExecuteContextSwitch(&next_task.TaskContext(), &current_task.TaskContext());
}

void TaskManager::Sleep(int task_id)
{
	auto task = running_tasks_.front();
	if (task->ID() == task_id) {
		SwitchTask(true);
		return;
	}

	for (auto it = running_tasks_.begin(); it != running_tasks_.end(); ++it) {
		if ((*it)->ID() == task_id) {
			running_tasks_.erase(it);
			wait_tasks_.emplace_back(*it);
			return;
		}
	}
}

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

void Task4()
{
	int i = 0;
	while (true) {
		char count[14];

		sprintf(count, "%d", i);

		Point2D draw_area = { static_cast<int>(150),
							  static_cast<int>(screen->Height() -
											   screen->Height() * 0.08) };

		screen->FillRectangle(draw_area,
							  { bitmap_font->Width() * 8, bitmap_font->Height() },
							  screen->TaskbarColor().GetCode());

		screen->DrawString(draw_area, count, 0xffffff);

		i++;
	}
}

TaskManager* task_manager;

void InitializeTaskManager()
{
	task_manager = new TaskManager();

	task_manager->AddTask(reinterpret_cast<uint64_t>(Task2), true);
	task_manager->AddTask(reinterpret_cast<uint64_t>(Task3), true);
	auto id = task_manager->AddTask(reinterpret_cast<uint64_t>(Task4), true);

	task_manager->Sleep(id);

	timer->AddSwitchTaskEvent(kSwitchTextMillisec);
}