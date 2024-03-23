#pragma once

#include "../file_system/file_descriptor.hpp"
#include "../list.hpp"
#include "../memory/slab.hpp"
#include "../task/context.hpp"
#include "../task/ipc.hpp"
#include "../types.hpp"
#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
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
	uint64_t kernel_stack_top;
	alignas(16) context ctx;
	list_elem_t run_queue_elem;
	std::queue<message> messages;
	std::array<std::function<void(const message&)>, NUM_MESSAGE_TYPES>
			message_handlers;
	std::array<std::shared_ptr<file_descriptor>, 10> fds;

	task(int id,
		 const char* task_name,
		 uint64_t task_addr,
		 task_state state,
		 int priority,
		 bool is_init);

	~task() { kfree(reinterpret_cast<void*>(ctx.cr3)); }

	static void* operator new(size_t size) { return kmalloc(size, KMALLOC_ZEROED); }

	static void operator delete(void* p) { kfree(p); }
};

extern task* CURRENT_TASK;
extern task* IDLE_TASK;

static constexpr int MAX_TASKS = 100;
extern std::array<task*, MAX_TASKS> tasks;
extern list_t run_queue;

task* create_task(const char* name, uint64_t task_addr, int priority, bool is_init);
task* get_scheduled_task();
task_t get_task_id_by_name(const char* name);
task_t get_available_task_id();
void schedule_task(task_t id);
void switch_task(const context& current_ctx);
[[noreturn]] void process_messages(task* t);
fd_t allocate_fd(task* t);

[[noreturn]] void task_idle();
