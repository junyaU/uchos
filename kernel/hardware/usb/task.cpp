#include "../../task/task.hpp"
#include "../../graphics/terminal.hpp"
#include "../../types.hpp"
#include "xhci/xhci.hpp"

void task_usb_handler()
{
	task* current_task = CURRENT_TASK;

	while (true) {
		__asm__("cli");
		if (current_task->messages.empty()) {
			__asm__("sti\n\thlt");
			continue;
		}

		const message m = current_task->messages.front();
		current_task->messages.pop();

		switch (m.type) {
			case NOTIFY_XHCI:
				usb::xhci::process_events();
				break;
		}
	}
}
