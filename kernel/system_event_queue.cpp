#include "system_event_queue.hpp"

#include "graphics/screen.hpp"
#include "graphics/system_logger.hpp"
#include "system_event.hpp"
#include "task/context.hpp"
#include "task/task.h"
#include "timers/timer.hpp"

#include <cstdio>

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
				system_logger->Print("Timer timeout\n");
				break;

			case SystemEvent::kDrawScreenTimer:
				char timer_value[14];
				sprintf(timer_value, "%lu",
						event.args_.draw_screen_timer.value / kTimerFrequency);
				DrawTimer(timer_value);
				break;

			default:
				system_logger->Printf("Unknown event type: %d\n", event.type_);
				break;
		}
	}
}