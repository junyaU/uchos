#pragma once

#include "context.hpp"
#include <cstddef>
#include <cstdint>
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

	int AddTask(uint64_t entry_point);

	void SwitchTask();

private:
	int last_task_id_;
	int current_task_index_;
	std::vector<std::unique_ptr<Task>> tasks_;
};

extern TaskManager* task_manager;

void InitializeTaskManager();
