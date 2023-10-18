#include "timer.hpp"

#include "system_event_queue.hpp"

#include "graphics/kernel_logger.hpp"
#include "task/task_manager.hpp"

uint64_t Timer::CalculateTimeoutTicks(unsigned long millisec)
{
	return tick_ + (millisec * kTimerFrequency) / 1000;
}

uint64_t Timer::AddTimerEvent(unsigned long millisec)
{
	auto event = SystemEvent{ SystemEvent::kTimerTimeout };

	event.args_.timer.id = last_id_;
	event.args_.timer.timeout = CalculateTimeoutTicks(millisec);
	event.args_.timer.period = millisec;

	events_.push(event);

	last_id_++;

	return event.args_.timer.id;
}

uint64_t Timer::AddPeriodicTimerEvent(unsigned long millisec, uint64_t id)
{
	auto event = SystemEvent{ SystemEvent::kTimerTimeout };

	if (id == 0) {
		id = last_id_;
		last_id_++;
	} else if (last_id_ < id) {
		klogger->printf("invalid timer id: %lu\n", id);
		return 0;
	}

	event.args_.timer.id = id;
	event.args_.timer.timeout = CalculateTimeoutTicks(millisec);
	event.args_.timer.period = millisec;
	event.args_.timer.periodical = 1;

	events_.push(event);

	return event.args_.timer.id;
}

void Timer::AddSwitchTaskEvent(unsigned long millisec)
{
	auto event = SystemEvent{ SystemEvent::kSwitchTask };

	event.args_.timer.timeout = CalculateTimeoutTicks(millisec);

	events_.push(event);
}

void Timer::RemoveTimerEvent(uint64_t id)
{
	if (id == 0 || last_id_ < id) {
		klogger->printf("invalid timer id: %lu\n", id);
		return;
	}

	ignore_events_.insert(id);
}

bool Timer::IncrementTick()
{
	tick_++;

	if (tick_ % kTimerFrequency == 0) {
		__asm__("cli");
		events_.push(SystemEvent{ SystemEvent::kDrawScreenTimer, { { tick_ } } });
		__asm__("sti");
	}

	bool need_switch_task = false;
	while (!events_.empty() && events_.top().args_.timer.timeout <= tick_) {
		auto event = events_.top();
		events_.pop();

		if (event.type_ == SystemEvent::kSwitchTask) {
			need_switch_task = true;

			event.args_.timer.timeout =
					CalculateTimeoutTicks(task_manager->NextQuantum());

			__asm__("cli");
			events_.push(event);
			__asm__("sti");

			continue;
		}

		auto it = std::find(ignore_events_.begin(), ignore_events_.end(),
							event.args_.timer.id);
		if (it != ignore_events_.end()) {
			ignore_events_.erase(it);
			continue;
		}

		if (!system_event_queue->Queue(event)) {
			klogger->printf("failed to queue timer event: %lu\n",
							event.args_.timer.id);
		}

		if (event.args_.timer.periodical) {
			event.args_.timer.timeout =
					CalculateTimeoutTicks(event.args_.timer.period);

			__asm__("cli");
			events_.push(event);
			__asm__("sti");
		}
	}

	return need_switch_task;
}

Timer* timer;

void InitializeTimer() { timer = new Timer; }