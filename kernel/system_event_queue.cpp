#include "system_event_queue.hpp"
#include "graphics/kernel_logger.hpp"
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
		return SystemEvent{ SystemEvent::kEmpty, { { 0 } } };
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
		__asm__("cli");
		if (system_event_queue->Empty()) {
			__asm__("sti\n\thlt");
			continue;
		}

		auto event = system_event_queue->Dequeue();
		switch (event.type_) {
			case SystemEvent::kTimerTimeout:
				klogger->print("Timer timeout\n");
				break;

			case SystemEvent::kDrawScreenTimer:
				break;

			default:
				klogger->printf("Unknown event type: %d\n", event.type_);
				break;
		}
	}
}