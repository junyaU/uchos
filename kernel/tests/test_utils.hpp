/**
 * @file tests/test_utils.hpp
 * @brief Shared helpers for kernel test cases
 */

#pragma once

#include "task/task.hpp"

namespace kernel::tests
{

/**
 * @brief Impersonate a task as CURRENT_TASK for a scope
 *
 * The impersonated task must be RUNNING while it is current: if a timer
 * preemption hits while CURRENT_TASK is WAITING, switch_task() does not
 * requeue it and the test runner's execution context is lost for good —
 * the boot hangs with no output (issue #313). RAII also restores
 * CURRENT_TASK when an ASSERT macro returns early.
 */
class ScopedCurrentTask
{
public:
	explicit ScopedCurrentTask(kernel::task::Task* t) : prev_(CURRENT_TASK)
	{
		t->state = kernel::task::TASK_RUNNING;
		CURRENT_TASK = t;
	}

	~ScopedCurrentTask() { CURRENT_TASK = prev_; }

	ScopedCurrentTask(const ScopedCurrentTask&) = delete;
	ScopedCurrentTask& operator=(const ScopedCurrentTask&) = delete;

private:
	kernel::task::Task* prev_; ///< CURRENT_TASK on entry
};

} // namespace kernel::tests
