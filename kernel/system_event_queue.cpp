#include "system_event_queue.hpp"
#include "graphics/kernel_logger.hpp"
#include "hardware/usb/xhci/xhci.hpp"
#include "system_event.hpp"

bool kernel_event_queue::queue(SystemEvent event)
{
	if (events_.size() >= QUEUE_SIZE) {
		return false;
	}

	events_.push(event);
	return true;
}

SystemEvent kernel_event_queue::dequeue()
{
	if (events_.empty()) {
		return SystemEvent{ SystemEvent::kEmpty, { { 0 } } };
	}

	auto event = events_.front();
	events_.pop();
	return event;
}

kernel_event_queue* kevent_queue;

void initialize_system_event_queue() { kevent_queue = new kernel_event_queue(); }

void handle_system_events()
{
	while (true) {
		__asm__("cli");
		if (kevent_queue->empty()) {
			__asm__("sti\n\thlt");
			continue;
		}

		auto event = kevent_queue->dequeue();
		switch (event.type_) {
			case SystemEvent::kTimerTimeout:
				klogger->print("Timer timeout\n");
				break;

			case SystemEvent::kDrawScreenTimer:
				break;

			case SystemEvent::XHCI:
				klogger->print("XHCI\n");
				usb::xhci::process_events();
				break;

			default:
				klogger->printf("Unknown event type: %d\n", event.type_);
				break;
		}
	}
}