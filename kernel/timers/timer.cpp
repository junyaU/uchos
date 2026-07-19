#include "timers/timer.hpp"
#include <cstdint>
#include <libs/common/message.hpp>
#include <libs/common/process_id.hpp>
#include <libs/common/types.hpp>
#include "log/log.hpp"
#include "memory/slab.hpp"
#include "task/ipc.hpp"

namespace
{
constexpr int SWITCH_TASK_MILLISEC = 20;
}

namespace kernel::timers
{

uint64_t KernelTimer::calculate_timeout_ticks(unsigned long millisec) const
{
	return tick_ + (millisec * TIMER_FREQUENCY) / 1000;
}

uint64_t KernelTimer::add_timer_event(unsigned long millisec,
									  TimeoutAction action,
									  ProcessId task_id)
{
	auto e = TimerEvent{
		.id = last_id_++,
		.task_id = task_id,
		.timeout = calculate_timeout_ticks(millisec),
		.period = static_cast<unsigned int>(millisec),
		.action = action,
	};

	events_.push(e);

	return e.id;
}

uint64_t KernelTimer::add_periodic_timer_event(unsigned long millisec,
											   TimeoutAction action,
											   ProcessId task_id,
											   uint64_t id)
{
	if (id == 0) {
		id = last_id_++;
	} else if (last_id_ < id) {
		LOG_ERROR("invalid timer id: %lu", id);
		return 0;
	}

	auto e = TimerEvent{
		.id = id,
		.task_id = task_id,
		.timeout = calculate_timeout_ticks(millisec),
		.period = static_cast<unsigned int>(millisec),
		.periodical = 1,
		.action = action,
	};

	events_.push(e);

	return e.id;
}

uint64_t KernelTimer::add_switch_task_event(unsigned long millisec)
{
	auto e = TimerEvent{
		.id = last_id_++,
		.task_id = process_ids::KERNEL, // Switch task events use kernel task
		.timeout = calculate_timeout_ticks(millisec),
		.period = 0,
		.periodical = 0,
		.action = TimeoutAction::SWITCH_TASK,
	};
	events_.push(e);

	return e.id;
}

error_t KernelTimer::remove_timer_event(uint64_t id)
{
	if (id == 0 || last_id_ < id) {
		LOG_ERROR("invalid timer id: %lu", id);
		return ERR_INVALID_ARG;
	}

	ignore_events_.insert(id);

	return OK;
}

bool KernelTimer::increment_tick()
{
	++tick_;

	bool need_switch_task = false;
	while (!events_.empty() && events_.top().timeout <= tick_) {
		auto e = events_.top();
		events_.pop();

		// Removal is lazy: remove_timer_event() only records the id, and the
		// queued entry is discarded here when it expires. Checked before the
		// SWITCH_TASK branch so a removed switch event neither switches nor
		// re-arms itself.
		if (ignore_events_.erase(e.id) > 0) {
			continue;
		}

		if (e.action == TimeoutAction::SWITCH_TASK) {
			need_switch_task = true;

			e.timeout = calculate_timeout_ticks(SWITCH_TASK_MILLISEC);
			events_.push(e);

			continue;
		}

		// increment_tick runs in the timer interrupt, so the expiry is
		// delivered as a doorbell bit: no allocation, and repeated
		// expiries coalesce instead of overflowing the receiver's ring
		// (issue #314 Stage A). The TimeoutAction payload is gone; the
		// receiver knows what its own timer means (cursor blink).
		kernel::task::notify(e.task_id, kernel::task::NotifyType::TIMER);

		if (e.periodical == 1) {
			e.timeout = calculate_timeout_ticks(e.period);
			events_.push(e);
		}
	}

	return need_switch_task;
}

KernelTimer* ktimer;

void initialize()
{
	LOG_INFO("Initializing logical timer...");

	void* addr;
	ALLOC_OR_RETURN(addr, sizeof(KernelTimer), kernel::memory::ALLOC_ZEROED);

	ktimer = new (addr) KernelTimer;

	LOG_INFO("Logical timer initialized successfully.");
}

} // namespace kernel::timers
