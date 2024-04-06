#pragma once

#include "file_system/file_descriptor.hpp"
#include "list.hpp"
#include "memory/paging.hpp"
#include "memory/slab.hpp"
#include "task/context.hpp"
#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <libs/common/message.hpp>
#include <libs/common/types.hpp>
#include <memory>
#include <queue>
#include <vector>

void initialize_task();

enum task_state : uint8_t { TASK_RUNNING, TASK_READY, TASK_WAITING, TASK_EXITED };

struct task {
	task_t id;
	task_t parent_id;
	char name[32];
	int priority;
	task_state state;
	std::vector<uint64_t> stack;
	uint64_t kernel_stack_ptr;
	alignas(16) context ctx;
	page_table_entry* page_table_snapshot;
	list_elem_t run_queue_elem;
	std::queue<message> messages;
	std::array<std::function<void(const message&)>, NUM_MESSAGE_TYPES>
			message_handlers;
	std::array<std::shared_ptr<file_descriptor>, 10> fds;

	task(int id,
		 const char* task_name,
		 uint64_t task_addr,
		 task_state state,
		 bool is_init);

	~task()
	{
		// clean_page_tables(reinterpret_cast<page_table_entry*>(ctx.cr3));
	}

	static void* operator new(size_t size) { return kmalloc(size, KMALLOC_ZEROED); }

	static void operator delete(void* p) { kfree(p); }

	error_t copy_parent_stack(const context& parent_ctx);

	error_t copy_parent_page_table();
};

extern task* CURRENT_TASK;
extern task* IDLE_TASK;

static constexpr int MAX_TASKS = 100;
extern std::array<task*, MAX_TASKS> tasks;
extern list_t run_queue;

task* create_task(const char* name, uint64_t task_addr, bool is_init);
task* copy_task(task* parent, context* parent_ctx);
task* get_scheduled_task();
task_t get_task_id_by_name(const char* name);
task_t get_available_task_id();
void schedule_task(task_t id);
void switch_task(const context& current_ctx);
void exit_task();
[[noreturn]] void process_messages(task* t);
fd_t allocate_fd(task* t);

[[noreturn]] void task_idle();
