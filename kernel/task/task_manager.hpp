#pragma once

#include "context.hpp"
#include <cstddef>
#include <cstdint>
#include <queue>
#include <vector>

// stack 4kiB

class Task
{
public:
	Task(int id, uint64_t task_addr);

	int ID() const { return id_; }

	Context& TaskContext() { return context_; }

private:
	int id_;
	std::vector<uint64_t> stack_;
	alignas(16) Context context_;
};

class TaskManager
{
public:
	TaskManager();

	int AddTask(uint64_t entry_point, bool is_running = false);

	void Wakeup(int task_id);
	void Sleep(int task_id);

	void SwitchTask(bool current_sleep = false);

private:
	int last_task_id_;
	std::vector<std::unique_ptr<Task>> wait_tasks_;
	std::deque<Task*> running_tasks_;
};

extern TaskManager* task_manager;

void InitializeTaskManager();
