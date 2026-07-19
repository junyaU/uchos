#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <libs/common/message.hpp>
#include <libs/common/process_id.hpp>
#include <libs/common/types.hpp>
#include "fs/file_descriptor.hpp"
#include "fs/path.hpp"
#include "list.hpp"
#include "memory/paging.hpp"
#include "memory/slab.hpp"
#include "task/context.hpp"
#include "task/message_queue.hpp"

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

/**
 * @brief Why a TASK_WAITING task is asleep; gates who may wake it
 *
 * send_message() wakes RECEIVE and NONE waiters; notify() additionally
 * wakes a NOTIFY waiter only when the raised bit is in its mask; a reply
 * wakes only the REPLY waiter it correlates with; a child's exit wakes
 * only a CHILD waiter. The gating keeps device waits and RPC waits from
 * being resumed spuriously by unrelated traffic (issue #314).
 */
enum class WaitReason : uint8_t {
	NONE,	 ///< Bare sleep via switch_next_task(true); any sender wakes it
	RECEIVE, ///< Blocked in a receive; messages and notifications wake it
	NOTIFY,	 ///< Blocked in wait_notification(); only masked doorbells wake it
	REPLY,	 ///< Blocked in call(); only the matching reply wakes it
	CHILD,	 ///< Blocked in sys_wait; only a child's exit wakes it
};

/**
 * @brief One child's exit, parked on the parent until sys_wait collects it
 *
 * Replaces the IPC_EXIT_TASK message: child termination is parent/child
 * bookkeeping, not IPC, so it no longer competes with real messages for
 * ring slots (issue #314 Stage B).
 */
struct ChildExitRecord {
	pid_t pid;
	int status;
};

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
	MessageQueue messages;
	uint32_t pending_notifications; ///< Sticky doorbell bits (see NotifyType)
	WaitReason wait_reason;			///< Valid while state == TASK_WAITING
	uint32_t wait_notify_mask;		///< Doorbells awaited (WaitReason::NOTIFY)
	uint32_t next_correlation;		///< call() id counter (0 is never issued)
	uint32_t call_correlation;		///< Outstanding call's id, 0 = no call
	bool reply_pending;				///< reply_slot holds an undelivered reply
	Message reply_slot;				///< Reply delivery slot, bypasses the ring
	static constexpr int MAX_CHILD_EXIT_RECORDS = 8;
	std::array<ChildExitRecord, MAX_CHILD_EXIT_RECORDS> exit_records;
	int num_exit_records; ///< Occupied prefix of exit_records (FIFO)
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

	/**
	 * @brief Run the registered handler for a message
	 *
	 * The single dispatch point shared by process_messages() and the FS
	 * init backlog replay, so no caller needs to touch the handler table
	 * (or the queue) directly.
	 */
	void dispatch_message(const Message& m);

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

/**
 * @brief Park a child's exit on the parent and wake a waiting sys_wait
 *
 * Appends to the parent's exit_records (FIFO; overflow is dropped with a
 * log) and wakes the parent when it is blocked in WAITING(CHILD).
 */
void record_child_exit(Task* parent, ProcessId child, int status);

[[noreturn]] void process_messages(Task* t);

} // namespace kernel::task

// For assembly and extern "C" compatibility
using kernel::task::CURRENT_TASK;
