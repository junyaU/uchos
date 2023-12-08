#include "task_manager.hpp"
#include "../graphics/terminal.hpp"
#include "../timers/timer.hpp"
#include "context_switch.h"
#include "task.hpp"

task_manager::task_manager() : last_task_id_{ 0 }
{
	tasks_.emplace_back(new Task(last_task_id_, 0, true, 2));
	++last_task_id_;
}

int task_manager::add_task(uint64_t task_addr, int priority, bool is_running)
{
	void* addr = kmalloc(sizeof(Task));
	Task* task = new (addr) Task(last_task_id_, task_addr, is_running, priority);

	tasks_.emplace_back(task);

	++last_task_id_;

	return task->ID();
}

void task_manager::switch_task(bool current_sleep)
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

void task_manager::sleep(int task_id)
{
	auto task = *tasks_.front();
	if (task.ID() == task_id) {
		switch_task(true);
		return;
	}

	auto it = std::find_if(tasks_.begin(), tasks_.end(),
						   [task_id](const auto& t) { return t->ID() == task_id; });
	if (it == tasks_.end()) {
		main_terminal->printf("TaskManager::Sleep: task ID is not running: %d\n",
							  task_id);
		return;
	}

	(*it)->Sleep();
}

void task_manager::wakeup(int task_id)
{
	if (last_task_id_ < task_id) {
		main_terminal->printf("TaskManager::Wakeup: invalid task ID: %d\n", task_id);
		return;
	}

	auto it = std::find_if(tasks_.begin(), tasks_.end(),
						   [task_id](const auto& t) { return t->ID() == task_id; });
	if (it == tasks_.end()) {
		main_terminal->printf("TaskManager::Wakeup: no such task ID: %d\n", task_id);
		return;
	}

	if ((*it)->IsRunning()) {
		main_terminal->printf("TaskManager::Wakeup: task %d is already running\n",
							  task_id);
		return;
	}

	(*it)->Wakeup();
}

int task_manager::next_quantum()
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

task_manager* ktask_manager;

void initialize_task_manager()
{
	ktask_manager = new task_manager();

	ktimer->add_switch_task_event(SWITCH_TEXT_MILLISEC);
}