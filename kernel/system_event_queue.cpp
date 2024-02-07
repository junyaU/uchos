#include "system_event_queue.hpp"
#include "graphics/terminal.hpp"
#include "hardware/usb/xhci/xhci.hpp"
#include "system_event.hpp"
#include "task/ipc.hpp"
#include "task/task.hpp"

bool kernel_event_queue::queue(system_event event)
{
	if (events_.size() >= QUEUE_SIZE) {
		return false;
	}

	events_.push(event);
	return true;
}

system_event kernel_event_queue::dequeue()
{
	if (events_.empty()) {
		return system_event{ system_event::EMPTY, { { 0 } } };
	}

	auto event = events_.front();
	events_.pop();
	return event;
}

kernel_event_queue* kevent_queue;

void initialize_system_event_queue() { kevent_queue = new kernel_event_queue(); }

[[noreturn]] void handle_system_events()
{
	while (true) {
		__asm__("cli");
		if (kevent_queue->empty()) {
			__asm__("sti\n\thlt");
			continue;
		}

		auto event = kevent_queue->dequeue();
		switch (event.type_) {
			case system_event::TIMER_TIMEOUT:
				switch (event.args_.timer.action) {
					case action_type::TERMINAL_CURSOR_BLINK:
						const message m = { NOTIFY_CURSOR_BLINK, CURRENT_TASK->id };
						send_message(main_terminal->task_id(), &m);
						break;
				}

				break;

			default:
				main_terminal->printf("Unknown event type: %d\n", event.type_);
				break;
		}
	}
}