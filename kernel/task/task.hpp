#pragma once

#include "../list.hpp"
#include "../memory/slab.hpp"
#include "../types.hpp"
#include "context.hpp"
#include "ipc.hpp"
#include <array>
#include <cstddef>
#include <cstdint>
#include <queue>
#include <vector>

void initialize_task();

enum task_state : uint8_t { TASK_RUNNING, TASK_READY, TASK_WAITING };

struct task {
	task_t id;
	char name[32];
	int priority;
	task_state state;
	std::vector<uint64_t> stack;
	alignas(16) context ctx;
	list_elem_t run_queue_elem;
	std::queue<message> messages;
	std::array<std::function<void(const message&)>, NUM_MESSAGE_TYPES>
			message_handlers;

	task(int id,
		 const char* task_name,
		 uint64_t task_addr,
		 task_state state,
		 int priority,
		 bool is_init);

	static void* operator new(size_t size) { return kmalloc(size, KMALLOC_ZEROED); }

	static void operator delete(void* p) { kfree(p); }
};

extern task* CURRENT_TASK;
extern task* IDLE_TASK;

static constexpr int MAX_TASKS = 10;
extern std::array<task*, MAX_TASKS> tasks;
extern list_t run_queue;

task* create_task(const char* name, uint64_t task_addr, int priority, bool is_init);
task* get_scheduled_task();
task_t get_task_id_by_name(const char* name);
task_t get_available_task_id();
void schedule_task(task_t id);
void switch_task(const context& current_ctx);
void process_messages(task* t);

void task_idle();
