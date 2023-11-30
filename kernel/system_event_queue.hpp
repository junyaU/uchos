#pragma once

#include "system_event.hpp"
#include <queue>

class kernel_event_queue
{
public:
	kernel_event_queue() = default;

	bool queue(SystemEvent event);
	SystemEvent dequeue();
	bool empty() const { return events_.empty(); }

private:
	std::queue<SystemEvent> events_;
	static const int QUEUE_SIZE = 1000;
};

extern kernel_event_queue* kevent_queue;

void initialize_system_event_queue();

void handle_system_events();
