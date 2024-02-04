#pragma once

#include "../list.hpp"
#include "../memory/slab.hpp"
#include "../types.hpp"
#include "context.hpp"
#include <cstdint>
#include <deque>
#include <memory>

void initialize_task_manager();

struct task {
	task_t id;
	char name[32];
	int priority;
	bool is_running;
	std::vector<uint64_t> stack;
	alignas(16) context ctx;
	list_elem_t run_queue_elem;

	task(int id, uint64_t task_addr, bool is_running, int priority, bool is_init);
};

extern task* CURRENT_TASK;
// task* IDLE_TASK = nullptr;

extern int last_task_id_;
extern list_t run_queue_list_;

task* add_task(uint64_t task_addr,
			   int priority,
			   bool is_init,
			   bool is_running = false);
void switch_task(const context& current_ctx);

task* get_scheduled_task();

void task_a();
