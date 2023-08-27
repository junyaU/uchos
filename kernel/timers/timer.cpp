#include "timer.hpp"

#include "system_event_queue.hpp"

#include "graphics/system_logger.hpp"

void Timer::AddTimerEvent(unsigned long millisec)
{
	auto event = SystemEvent{ SystemEvent::kTimerTimeout };
	event.args_.timer.timeout = tick_ + (millisec * kFrequency) / 1000;
	events_.push(event);
}

void Timer::IncrementTick()
{
	tick_++;

	while (!events_.empty() && events_.top().args_.timer.timeout <= tick_) {
		auto event = events_.top();
		events_.pop();

		if (!system_event_queue->Queue(event)) {
			system_logger->Print("failed to queue system event\n");
		}
	}
}

Timer* timer;

void InitializeTimer() { timer = new Timer; }