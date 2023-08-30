#include "timer.hpp"

#include "system_event_queue.hpp"

#include "graphics/system_logger.hpp"

uint64_t Timer::AddTimerEvent(unsigned long millisec, bool periodic)
{
	auto event = SystemEvent{
		periodic ? SystemEvent::kTimerPeriodicTimeout : SystemEvent::kTimerTimeout,
	};

	event.args_.timer.id = last_id_++;
	event.args_.timer.timeout = tick_ + (millisec * kFrequency) / 1000;
	event.args_.timer.period = millisec;

	events_.push(event);

	return event.args_.timer.id;
}

void Timer::RemoveTimerEvent(uint64_t id)
{
	if (id == 0 || last_id_ < id) {
		system_logger->Printf("invalid timer id: %lu\n", id);
		return;
	}

	ignore_events_.insert(id);
}

void Timer::IncrementTick()
{
	tick_++;

	while (!events_.empty() && events_.top().args_.timer.timeout <= tick_) {
		auto event = events_.top();
		events_.pop();

		auto it = std::find(ignore_events_.begin(), ignore_events_.end(),
							event.args_.timer.id);
		if (it != ignore_events_.end()) {
			ignore_events_.erase(it);
			continue;
		}

		if (!system_event_queue->Queue(event)) {
			system_logger->Printf("failed to queue timer event: %lu\n",
								  event.args_.timer.id);
		}
	}
}

Timer* timer;

void InitializeTimer() { timer = new Timer; }