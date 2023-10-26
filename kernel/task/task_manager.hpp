#pragma once

class Task;

#include <cstdint>
#include <deque>
#include <memory>

class TaskManager
{
public:
	TaskManager();

	int AddTask(uint64_t task_addr, int priority, bool is_running = false);

	void Wakeup(int task_id);
	void Sleep(int task_id);

	void SwitchTask(bool current_sleep = false);

	int NextQuantum();

private:
	int last_task_id_;
	std::deque<std::unique_ptr<Task>> tasks_;
};

extern TaskManager* task_manager;

void InitializeTaskManager();
