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

Task::Task(int id, uint64_t task_addr, bool is_runnning, int priority)
	: id_{ id },
	  priority_{ priority },
	  is_running_{ is_runnning },
	  stack_{ 4096 },
	  context_{ 0 }
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
	tasks_.emplace_back(new Task(last_task_id_, 0, true, 2));
	++last_task_id_;
}

int TaskManager::AddTask(uint64_t task_addr, int priority, bool is_running)
{
	Task* task = new Task(last_task_id_, task_addr, is_running, priority);

	tasks_.emplace_back(task);

	++last_task_id_;

	return task->ID();
}

void TaskManager::SwitchTask(bool current_sleep)
{
	if (tasks_.size() <= 1) {
		return;
	}

	Task& current_task = *tasks_.front();

	if (current_sleep) {
		current_task.Sleep();
	}

	tasks_.push_back(std::move(tasks_.front()));
	tasks_.pop_front();

	Task& next_task = *tasks_.front();

	ExecuteContextSwitch(&next_task.TaskContext(), &current_task.TaskContext());
}

void TaskManager::Sleep(int task_id)
{
	auto task = *tasks_.front();
	if (task.ID() == task_id) {
		SwitchTask(true);
		return;
	}

	auto it = std::find_if(tasks_.begin(), tasks_.end(),
						   [task_id](const auto& t) { return t->ID() == task_id; });
	if (it == tasks_.end()) {
		system_logger->Printf("TaskManager::Sleep: task ID is not running: %d\n",
							  task_id);
		return;
	}

	(*it)->Sleep();
}

void TaskManager::Wakeup(int task_id)
{
	if (last_task_id_ < task_id) {
		system_logger->Printf("TaskManager::Wakeup: invalid task ID: %d\n", task_id);
		return;
	}

	auto it = std::find_if(tasks_.begin(), tasks_.end(),
						   [task_id](const auto& t) { return t->ID() == task_id; });
	if (it == tasks_.end()) {
		system_logger->Printf("TaskManager::Wakeup: no such task ID: %d\n", task_id);
		return;
	}

	if ((*it)->IsRunning()) {
		system_logger->Printf("TaskManager::Wakeup: task %d is already running\n",
							  task_id);
		return;
	}

	(*it)->Wakeup();
}

int TaskManager::NextQuantum()
{
	auto it = std::find_if(tasks_.begin() + 1, tasks_.end(),
						   [](const auto& t) { return t->IsRunning(); });
	if (it == tasks_.end()) {
		return 0;
	}

	switch ((*it)->Priority()) {
		case 2:
			return 80;
		case 1:
			return 50;
		case 0:
			return 20;
		default:
			return 0;
	}
}

void TaskA()
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

void TaskB()
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

	task_manager->AddTask(reinterpret_cast<uint64_t>(TaskA), 2, true);
	task_manager->AddTask(reinterpret_cast<uint64_t>(TaskB), 2, true);

	timer->AddSwitchTaskEvent(kSwitchTextMillisec);
}