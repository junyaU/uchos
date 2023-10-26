#pragma once

#include "system_event.hpp"
#include <queue>

class SystemEventQueue
{
public:
	SystemEventQueue(){};
	bool Queue(SystemEvent event);
	SystemEvent Dequeue();
	bool Empty() const { return events_.empty(); }

private:
	static const int kQueueSize = 1000;
	std::queue<SystemEvent> events_;
};

extern SystemEventQueue* system_event_queue;

void HandleSystemEvents();