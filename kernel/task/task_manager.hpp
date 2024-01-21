#pragma once

class Task;

#include "../memory/slab.hpp"
#include "context.hpp"
#include <cstdint>
#include <deque>
#include <memory>

class task_manager
{
public:
	task_manager();

	int add_task(uint64_t task_addr, int priority, bool is_running = false);

	void wakeup(int task_id);
	void sleep(int task_id);

	void switch_task(const context& current_ctx);

	int next_quantum();

private:
	int last_task_id_;
	std::deque<std::unique_ptr<Task, kfree_deleter>> tasks_;
};

extern task_manager* ktask_manager;

void initialize_task_manager();
