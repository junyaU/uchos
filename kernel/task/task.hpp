#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <libs/common/message.hpp>
#include <libs/common/process_id.hpp>
#include <libs/common/types.hpp>
#include "fs/file_descriptor.hpp"
#include "fs/path.hpp"
#include "list.hpp"
#include "memory/paging.hpp"
#include "memory/slab.hpp"
#include "task/context.hpp"

namespace kernel::task
{

void initialize();

/**
 * @brief Start preemptive scheduling by arming the switch-task timer
 *
 * Kept separate from initialize() so the boot flow controls exactly when
 * task switching begins. The main-stage test suites run between the two:
 * they must not be preempted, because a service task running mid-suite
 * corrupts the per-suite leak accounting and can hit half-initialized
 * state (this was a real CI flake, three failure modes from one race).
 */
void start_scheduling();

enum TaskState : uint8_t { TASK_RUNNING, TASK_READY, TASK_WAITING, TASK_EXITED };

static constexpr int MAX_FDS_PER_PROCESS = 32;

struct Task {
	ProcessId id;
	ProcessId parent_id;
	char name[32];
	int priority;
	bool is_initialized;
	TaskState state;
	Path fs_path;
	uint64_t* stack;
	size_t stack_size;
	uint64_t kernel_stack_ptr;
	alignas(16) Context ctx;
	list_elem_t run_queue_elem;
	std::deque<Message> messages;
	std::array<message_handler_t, TOTAL_MESSAGE_TYPES> message_handlers;
	std::array<kernel::fs::FileDescriptor, MAX_FDS_PER_PROCESS> fd_table;

	Task(int id,
		 const char* task_name,
		 uint64_t task_addr,
		 TaskState state,
		 bool setup_context,
		 bool is_initialized);

	~Task()
	{
		if (ctx.cr3 != 0) {
			kernel::memory::clean_page_tables(
					reinterpret_cast<kernel::memory::page_table_entry*>(ctx.cr3));
		}

		kernel::memory::free(stack);
	}

	static void* operator new(size_t size)
	{
		return kernel::memory::alloc(size, kernel::memory::ALLOC_ZEROED);
	}

	static void operator delete(void* p) { kernel::memory::free(p); }

	error_t copy_parent_stack(const Context& parent_ctx);

	error_t copy_parent_page_table();

	void add_msg_handler(MsgType type, message_handler_t handler);

	bool has_parent() const { return parent_id.raw() != -1; }

	kernel::memory::page_table_entry* get_page_table() const
	{
		return reinterpret_cast<kernel::memory::page_table_entry*>(ctx.cr3);
	}
};

struct InitialTaskInfo {
	SystemProcessId id; ///< tasks[] slot this entry must land in (asserted)
	const char* name;
	void (*entry)(); ///< Service entry point, nullptr for the boot task
	bool setup_context;
	bool is_initialized;
};

extern Task* CURRENT_TASK;
extern Task* IDLE_TASK;

static constexpr int MAX_TASKS = 100;
extern std::array<Task*, MAX_TASKS> tasks;
extern list_t run_queue;

Task* create_task(const char* name,
				  uint64_t task_addr,
				  bool setup_context,
				  bool is_initialized);

Task* copy_task(Task* parent, Context* parent_ctx);

/**
 * @brief Pop the next task to run from the run queue
 *
 * Falls back to the idle task when the run queue is empty. Updates
 * CURRENT_TASK and marks the popped task TASK_RUNNING as a side effect.
 *
 * @return The task that should run next
 */
Task* pick_next_task();

Task* get_task(ProcessId id);

ProcessId get_task_id_by_name(const char* name);

ProcessId get_available_task_id();

void schedule_task(ProcessId id);

void switch_task(const Context& current_ctx);

void switch_next_task(bool sleep_current_task);

void exit_task(int status);

[[noreturn]] void process_messages(Task* t);

Message wait_for_message(MsgType type);

} // namespace kernel::task

// For assembly and extern "C" compatibility
using kernel::task::CURRENT_TASK;
