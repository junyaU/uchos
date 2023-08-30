#include "system_event_queue.hpp"

#include "graphics/system_logger.hpp"
#include "system_event.hpp"
#include "timers/timer.hpp"

bool SystemEventQueue::Queue(SystemEvent event)
{
	if (events_.size() >= kQueueSize) {
		return false;
	}

	events_.push(event);
	return true;
}

SystemEvent SystemEventQueue::Dequeue()
{
	if (events_.empty()) {
		return SystemEvent{ SystemEvent::kEmpty, 0 };
	}

	auto event = events_.front();
	events_.pop();
	return event;
}

SystemEventQueue* system_event_queue;

void HandleSystemEvents()
{
	system_event_queue = new SystemEventQueue;

	while (true) {
		if (system_event_queue->Empty()) {
			continue;
		}

		auto event = system_event_queue->Dequeue();
		switch (event.type_) {
			case SystemEvent::kTimerTimeout:
				system_logger->Print("Timer timeout\n");
				break;

			case SystemEvent::kTimerPeriodicTimeout:
				timer->AddTimerEvent(event.args_.timer.period, true);
				system_logger->Print("Timer periodic timeout\n");
				break;

			default:
				break;
		}
	}
}