#pragma once

#include "../memory/slab.hpp"
#include "context.hpp"
#include <cstdint>
#include <deque>
#include <memory>

void initialize_task_manager();

class Task
{
public:
	Task(int id, uint64_t task_addr, bool is_runnning, int priority, bool is_init);

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

void task_a();
//
extern int last_task_id_;
extern std::deque<std::unique_ptr<Task>> tasks_;

int add_task(uint64_t task_addr,
			 int priority,
			 bool is_init,
			 bool is_running = false);
void switch_task(const context& current_ctx);
