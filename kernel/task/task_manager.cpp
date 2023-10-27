#include "task_manager.hpp"
#include "../graphics/kernel_logger.hpp"
#include "../timers/timer.hpp"
#include "context_switch.h"
#include "task.hpp"

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
		klogger->printf("TaskManager::Sleep: task ID is not running: %d\n", task_id);
		return;
	}

	(*it)->Sleep();
}

void TaskManager::Wakeup(int task_id)
{
	if (last_task_id_ < task_id) {
		klogger->printf("TaskManager::Wakeup: invalid task ID: %d\n", task_id);
		return;
	}

	auto it = std::find_if(tasks_.begin(), tasks_.end(),
						   [task_id](const auto& t) { return t->ID() == task_id; });
	if (it == tasks_.end()) {
		klogger->printf("TaskManager::Wakeup: no such task ID: %d\n", task_id);
		return;
	}

	if ((*it)->IsRunning()) {
		klogger->printf("TaskManager::Wakeup: task %d is already running\n",
						task_id);
		return;
	}

	(*it)->Wakeup();
}

int TaskManager::NextQuantum()
{
	int priority = 0;
	auto it = std::find_if(tasks_.begin() + 1, tasks_.end(),
						   [](const auto& t) { return t->IsRunning(); });
	if (it == tasks_.end()) {
		auto current_task = *tasks_.front();
		priority = current_task.Priority();
	} else {
		priority = (*it)->Priority();
	}

	switch (priority) {
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

TaskManager* task_manager;

void InitializeTaskManager()
{
	task_manager = new TaskManager();

	timer->AddSwitchTaskEvent(kSwitchTextMillisec);
}