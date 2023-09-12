#pragma once

#include "context.hpp"
#include <cstddef>
#include <cstdint>
#include <queue>
#include <vector>

class Task
{
public:
	Task(int id, uint64_t task_addr, bool is_runnning, int priority);

	int ID() const { return id_; }

	Context& TaskContext() { return context_; }

	void Sleep() { is_running_ = false; }

	void Wakeup() { is_running_ = true; }

	bool IsRunning() const { return is_running_; }

	int Priority() const { return priority_; }

private:
	int id_;
	int priority_;
	bool is_running_;
	std::vector<uint64_t> stack_;
	alignas(16) Context context_;
};

class TaskManager
{
public:
	TaskManager();

	int AddTask(uint64_t entry_point, int priority, bool is_running = false);

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
