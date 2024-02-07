#include "../../task/task.hpp"
#include "../../graphics/terminal.hpp"
#include "../../types.hpp"
#include "xhci/xhci.hpp"

void task_usb_handler()
{
	task* t = CURRENT_TASK;

	t->message_handlers[NOTIFY_XHCI] = [](const message& m) {
		usb::xhci::process_events();
	};

	process_messages(t);
}
