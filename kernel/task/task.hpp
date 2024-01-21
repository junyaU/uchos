#pragma once

#include <cstdint>
#include <vector>

#include "context.hpp"

class Task
{
public:
	Task(int id, uint64_t task_addr, bool is_runnning, int priority);

	int ID() const { return id_; }

	context& TaskContext() { return context_; }

	void Sleep() { is_running_ = false; }

	void Wakeup() { is_running_ = true; }

	bool IsRunning() const { return is_running_; }

	int Priority() const { return priority_; }

private:
	int id_;
	int priority_;
	bool is_running_;
	std::vector<uint64_t> stack_;
	alignas(16) context context_;
};
