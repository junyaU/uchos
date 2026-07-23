#include "task/task.hpp"
#include <array>
#include <cstdint>
#include <cstring>
#include <libs/common/message.hpp>
#include <libs/common/process_id.hpp>
#include <libs/common/types.hpp>
#include "fs/fat/fat.hpp"
#include "fs/path.hpp"
#include "hardware/virtio/blk.hpp"
#include "hardware/virtio/net.hpp"
#include "interrupt/irq_guard.hpp"
#include "interrupt/vector.hpp"
#include "list.hpp"
#include "log/log.hpp"
#include "memory/page.hpp"
#include "memory/paging.hpp"
#include "memory/paging_utils.h"
#include "memory/segment.hpp"
#include "memory/slab.hpp"
#include "net/packet_handler.hpp"
#include "task/builtin.hpp"
#include "task/context.hpp"
#include "task/context_switch.h"
#include "task/ipc.hpp"
#include "timers/timer.hpp"

namespace kernel::task
{

list_t run_queue;
std::array<Task*, MAX_TASKS> tasks;

Task* CURRENT_TASK = nullptr;
Task* IDLE_TASK = nullptr;

constexpr InitialTaskInfo INITIAL_TASKS[] = {
	{ SystemProcessId::KERNEL, "main", nullptr, false, true },
	{ SystemProcessId::IDLE, "idle", &kernel::task::idle_service, true, true },
	{ SystemProcessId::XHCI, "usb_handler", &kernel::task::usb_handler_service, true,
	  true },
	{ SystemProcessId::VIRTIO_BLK, "virtio_blk",
	  &kernel::hw::virtio::virtio_blk_service, true, false },
	{ SystemProcessId::FS_FAT32, "fat32", &kernel::fs::fat32_service, true, true },
	{ SystemProcessId::SHELL, "shell", &kernel::task::shell_service, true, false },
	{ SystemProcessId::VIRTIO_NET, "virtio_net",
	  &kernel::hw::virtio::virtio_net_service, true, false },
	{ SystemProcessId::NET, "net", &kernel::net::packet_handler_service, true,
	  false },
};

namespace
{
// create_task() hands out slots in ascending order, so each entry's declared
// SystemProcessId only holds if the array is ordered by those ids with no gap.
constexpr bool initial_tasks_match_their_slots()
{
	for (size_t i = 0; i < sizeof(INITIAL_TASKS) / sizeof(INITIAL_TASKS[0]); ++i) {
		if (static_cast<pid_t>(INITIAL_TASKS[i].id) != static_cast<pid_t>(i)) {
			return false;
		}
	}
	return true;
}
static_assert(initial_tasks_match_their_slots(),
			  "INITIAL_TASKS must be ordered by SystemProcessId, gap-free");
} // namespace

ProcessId get_available_task_id()
{
	for (pid_t i = 0; i < MAX_TASKS; i++) {
		if (tasks[i] == nullptr) {
			return ProcessId::from_raw(i);
		}
	}

	return ProcessId::from_raw(-1);
}

ProcessId get_task_id_by_name(const char* name)
{
	for (Task* t : tasks) {
		if (t != nullptr && strcmp(t->name, name) == 0) {
			return t->id;
		}
	}

	return ProcessId::from_raw(-1);
}

Task* get_task(ProcessId id)
{
	const pid_t raw_id = id.raw();
	if (raw_id < 0 || raw_id >= MAX_TASKS) {
		return nullptr;
	}

	return tasks[raw_id];
}

Task* create_task(const char* name,
				  uint64_t task_addr,
				  bool setup_context,
				  bool is_init)
{
	const ProcessId task_id = get_available_task_id();
	if (task_id.raw() == -1) {
		LOG_ERROR("failed to allocate task id");
		return nullptr;
	}

	tasks[task_id.raw()] = new Task(task_id.raw(), name, task_addr, TASK_WAITING,
									setup_context, is_init);

	return tasks[task_id.raw()];
}

error_t Task::copy_parent_stack(const Context& parent_ctx)
{
	Task* parent = tasks[parent_id.raw()];
	if (parent == nullptr) {
		return ERR_NO_TASK;
	}

	stack_size = parent->stack_size;

	void* stack_ptr;
	ALLOC_OR_RETURN_ERROR(stack_ptr, stack_size, kernel::memory::ALLOC_ZEROED);
	stack = static_cast<uint64_t*>(stack_ptr);

	memcpy(stack, parent->stack, stack_size);

	auto parent_stack_top = reinterpret_cast<uint64_t>(parent->stack) + stack_size;
	auto child_stack_top = reinterpret_cast<uint64_t>(stack) + stack_size;
	const uint64_t rsp_offset = parent_stack_top - parent_ctx.rsp;
	const uint64_t rbp_offset = parent_stack_top - parent_ctx.rbp;

	ctx.rsp = child_stack_top - rsp_offset;
	ctx.rbp = child_stack_top - rbp_offset;

	const uint64_t kernel_sp_offset = parent_stack_top - parent->kernel_stack_ptr;
	kernel_stack_ptr = child_stack_top - kernel_sp_offset;

	return OK;
}

error_t Task::copy_parent_page_table()
{
	Task* parent = tasks[parent_id.raw()];
	if (parent == nullptr) {
		return ERR_NO_TASK;
	}

	kernel::memory::page_table_entry* snapshot =
			reinterpret_cast<kernel::memory::page_table_entry*>(parent->ctx.cr3);

	kernel::memory::page_table_entry* parent_table =
			kernel::memory::clone_page_table(snapshot, false);
	if (parent_table == nullptr) {
		return ERR_NO_MEMORY;
	}

	// Move the running parent onto its CoW clone. Update ctx.cr3 too so an
	// exit before the next context save does not tear down the wrong table.
	set_cr3(reinterpret_cast<uint64_t>(parent_table));
	parent->ctx.cr3 = reinterpret_cast<uint64_t>(parent_table);

	kernel::memory::page_table_entry* child_table =
			kernel::memory::clone_page_table(snapshot, false);
	if (child_table == nullptr) {
		return ERR_NO_MEMORY;
	}

	ctx.cr3 = reinterpret_cast<uint64_t>(child_table);

	// Both clones hold their own references to the CoW data pages now, and
	// nothing runs on the pre-fork table anymore; release it (issue #313
	// page table leak).
	kernel::memory::clean_page_tables(snapshot);

	return OK;
}

void Task::add_msg_handler(MsgType type, message_handler_t handler)
{
	const int32_t index = static_cast<int32_t>(type);
	if (index < 0 || index >= TOTAL_MESSAGE_TYPES) {
		return;
	}
	message_handlers[index] = handler;
}

void Task::dispatch_message(const Message& m)
{
	const int32_t index = static_cast<int32_t>(m.type);
	if (index < 0 || index >= TOTAL_MESSAGE_TYPES) {
		return;
	}
	message_handlers[index](m);
}

Task* copy_task(Task* parent, Context* parent_ctx)
{
	Task* child = create_task(parent->name, 0, false, true);
	if (child == nullptr) {
		return nullptr;
	}
	child->parent_id = parent->id;

	memcpy(&child->ctx, parent_ctx, sizeof(Context));

	// Copy parent's file descriptor table
	if (IS_ERR(kernel::fs::copy_fd_table(child->fd_table.data(),
										 parent->fd_table.data(),
										 MAX_FDS_PER_PROCESS, child->id))) {
		LOG_ERROR("Failed to copy file descriptor table : %s", parent->name);
		return nullptr;
	}

	if (IS_ERR(child->copy_parent_stack(*parent_ctx))) {
		LOG_ERROR("Failed to copy parent stack : %s", parent->name);
		return nullptr;
	}

	if (IS_ERR(child->copy_parent_page_table())) {
		LOG_ERROR("Failed to copy parent page table : %s", parent->name);
		return nullptr;
	}

	// The child inherits no OOL ownership (its region table starts empty),
	// so drop the parent's mapped regions from the child's cloned table:
	// left in place, the parent's ool_release/exit would free pages the
	// child still maps (stale reads of reused kernel memory).
	for (const auto& r : parent->ool_regions) {
		if (r.kaddr != 0 && IS_ERR(kernel::memory::unmap_frame(
									child->get_page_table(),
									kernel::memory::vaddr_t{ r.uaddr }, r.pages))) {
			LOG_ERROR("failed to unmap inherited ool region: child %d",
					  child->id.raw());
		}
	}

	return child;
}

Task* pick_next_task()
{
	if (list_is_empty(&run_queue)) {
		CURRENT_TASK = IDLE_TASK;
		return IDLE_TASK;
	}

	Task* scheduled_task = LIST_POP_FRONT(&run_queue, Task, run_queue_elem);
	scheduled_task->state = TASK_RUNNING;
	CURRENT_TASK = scheduled_task;

	return scheduled_task;
}

void schedule_task(ProcessId id)
{
	const pid_t raw_id = id.raw();
	if (raw_id < 0 || tasks.size() <= static_cast<size_t>(raw_id) ||
		tasks[raw_id] == nullptr) {
		LOG_ERROR("schedule_task: task %d is not found", raw_id);
		return;
	}

	// The run queue is shared with interrupt handlers
	const kernel::interrupt::IrqGuard guard;

	tasks[raw_id]->state = TASK_READY;

	if (list_contains(&run_queue, &tasks[raw_id]->run_queue_elem)) {
		return;
	}

	list_push_back(&run_queue, &tasks[raw_id]->run_queue_elem);
}

namespace
{
// A task cannot tear itself down from switch_task: ~Task frees the kernel stack
// we are still executing on and the page tables the CPU is still translating
// through. Doing so is a use-after-free -- harmless by luck normally, but with
// KERNEL_HEAP_DEBUG the freed stack is poisoned and the running code faults on
// the next return. Instead we stash the exited task and delete it on the next
// switch, once we are on another task's stack and address space.
Task* task_pending_reap = nullptr;

void reap_pending_task()
{
	// Clear the slot before deleting so a switch that re-enters mid-delete
	// (e.g. a timer tick) cannot free the same task twice.
	Task* t = task_pending_reap;
	task_pending_reap = nullptr;
	delete t; // delete nullptr is a no-op
}
} // namespace

void switch_task(const Context& current_ctx)
{
	// Delete the task that exited on the previous switch. We are no longer on
	// its stack or in its address space, so freeing them is safe now.
	reap_pending_task();

	if (CURRENT_TASK->state == TASK_EXITED) {
		// Defer teardown to the next switch (see reap_pending_task); keep
		// interrupts off so nothing reuses the freed slot until we have
		// switched onto the next task's stack.
		asm volatile("cli");
		const pid_t exited_id = CURRENT_TASK->id.raw();
		task_pending_reap = tasks[exited_id];
		tasks[exited_id] = nullptr;
	} else {
		memcpy(&CURRENT_TASK->ctx, &current_ctx, sizeof(Context));
		if (CURRENT_TASK->state != TASK_WAITING) {
			schedule_task(CURRENT_TASK->id);
		}
	}

	restore_context(&pick_next_task()->ctx);
}

void switch_next_task(bool sleep_current_task)
{
	if (sleep_current_task) {
		// Bare sleep: no specific wake condition, so any sender may wake it
		CURRENT_TASK->wait_reason = WaitReason::NONE;
		CURRENT_TASK->state = TASK_WAITING;
	}

	asm("int %0" : : "i"(kernel::interrupt::InterruptVector::SWITCH_TASK));
}

void record_child_exit(Task* parent, ProcessId child, int status)
{
	const kernel::interrupt::IrqGuard guard;

	if (parent->num_exit_records >= Task::MAX_CHILD_EXIT_RECORDS) {
		LOG_ERROR("exit records full: parent = %d, child = %d dropped",
				  parent->id.raw(), child.raw());
		return;
	}

	parent->exit_records[parent->num_exit_records] =
			ChildExitRecord{ child.raw(), status };
	++parent->num_exit_records;

	if (parent->state == TASK_WAITING && parent->wait_reason == WaitReason::CHILD) {
		schedule_task(parent->id);
	}
}

void exit_task(int status)
{
	Task* t = CURRENT_TASK;

	// Release all file descriptors before exiting
	kernel::fs::release_all_process_fds(t->fd_table.data(), MAX_FDS_PER_PROCESS);

	// Free every OOL buffer this task still owns: mapped regions, queued
	// messages, an uncollected reply. The mappings die with the page table.
	release_all_ool(t);

	// Child termination is parent/child bookkeeping, not IPC (issue #314
	// Stage B): park the status on the parent for sys_wait to collect
	if (t->has_parent()) {
		Task* parent = get_task(t->parent_id);
		if (parent != nullptr) {
			record_child_exit(parent, t->id, status);
		}
	}

	t->state = TASK_EXITED;
	switch_task(t->ctx);
}

[[noreturn]] void process_messages(Task* t)
{
	while (true) {
		// Blocks until a message or doorbell arrives; the lost-wakeup
		// discipline lives in receive_blocking (issue #314 Stage A)
		const Message m = receive_blocking();
		t->dispatch_message(m);
	}
}

void initialize()
{
	tasks = std::array<Task*, MAX_TASKS>();
	list_init(&run_queue);

	for (const auto& t_info : INITIAL_TASKS) {
		Task* new_task =
				create_task(t_info.name, reinterpret_cast<uint64_t>(t_info.entry),
							t_info.setup_context, t_info.is_initialized);
		if (new_task != nullptr) {
			schedule_task(new_task->id);
		}
	}

	CURRENT_TASK = tasks[process_ids::KERNEL.raw()];
	IDLE_TASK = tasks[process_ids::IDLE.raw()];

	CURRENT_TASK->state = TASK_RUNNING;
}

void start_scheduling() { kernel::timers::ktimer->add_switch_task_event(200); }

namespace
{
// Number of PAGE_SIZE pages allocated for a task's stack.
constexpr size_t KERNEL_STACK_PAGES = 8;

// Initial RFLAGS for a new task: bit 1 is Intel-reserved and must always
// read as 1; bit 9 (IF) enables interrupts.
constexpr uint64_t RFLAGS_RESERVED1 = 1U << 1;
constexpr uint64_t RFLAGS_IF = 1U << 9;

// Default MXCSR value (all SSE floating-point exceptions masked).
constexpr uint32_t MXCSR_DEFAULT = 0x1f80;
} // namespace

Task::Task(int raw_id,
		   const char* task_name,
		   uint64_t task_addr,
		   TaskState state,
		   bool setup_context,
		   bool is_initialized)
	: id{ ProcessId::from_raw(raw_id) },
	  parent_id{ ProcessId::from_raw(-1) },
	  priority{ 2 }, // TODO: Implement priority scheduling
	  is_initialized{ is_initialized },
	  state{ state },
	  fs_path({ nullptr, nullptr, nullptr }),
	  stack{ nullptr },
	  pending_notifications{ 0 },
	  wait_reason{ WaitReason::NONE },
	  wait_notify_mask{ 0 },
	  next_correlation{ 0 },
	  call_correlation{ 0 },
	  reply_pending{ false },
	  reply_slot{},
	  exit_records{},
	  num_exit_records{ 0 },
	  ool_regions{},
	  message_handlers({ std::array<message_handler_t, TOTAL_MESSAGE_TYPES>() }),
	  fd_table()
{
	list_elem_init(&run_queue_elem);

	for (auto& handler : message_handlers) {
		handler = [](const Message&) {};
	}

	// Initialize file descriptor table
	kernel::fs::init_process_fd_table(fd_table.data(), MAX_FDS_PER_PROCESS);

	strncpy(name, task_name, sizeof(name) - 1);
	name[sizeof(name) - 1] = '\0';

	if (!setup_context) {
		return;
	}

	stack_size = kernel::memory::PAGE_SIZE * KERNEL_STACK_PAGES;
	void* stack_ptr;
	ALLOC_OR_RETURN(stack_ptr, stack_size, kernel::memory::ALLOC_ZEROED);
	stack = static_cast<uint64_t*>(stack_ptr);

	const uint64_t stack_end = reinterpret_cast<uint64_t>(stack) + stack_size;

	kernel::memory::page_table_entry* page_table = kernel::memory::new_page_table();
	kernel::memory::copy_kernel_space(page_table);

	ctx = {};
	ctx.cr3 = reinterpret_cast<uint64_t>(page_table);
	ctx.rsp = (stack_end & ~0xfLU) - 8;
	ctx.rflags = RFLAGS_RESERVED1 | RFLAGS_IF;
	ctx.rip = task_addr;
	ctx.cs = kernel::memory::KERNEL_CS;
	ctx.ss = kernel::memory::KERNEL_SS;
	*reinterpret_cast<uint32_t*>(&ctx.fxsave_area[24]) = MXCSR_DEFAULT;
}

} // namespace kernel::task

extern "C" uint64_t get_current_task_stack()
{
	return kernel::task::CURRENT_TASK->kernel_stack_ptr;
}
