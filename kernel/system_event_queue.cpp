#include "system_event_queue.hpp"

#include "system_event.hpp"

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
		return SystemEvent{};
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
		__asm__("hlt");
	}

	while (true) {
		if (system_event_queue->Empty()) {
			continue;
		}

		auto event = system_event_queue->Dequeue();
		switch (event.type_) {
			case SystemEvent::kTimerTimeout:
				break;
			default:
				break;
		}
	}
}