#include "hardware/usb_service.hpp"
#include <libs/common/message.hpp>
#include "hardware/keyboard.hpp"
#include "hardware/pci.hpp"
#include "hardware/usb/xhci/xhci.hpp"
#include "task/ipc.hpp"
#include "task/task.hpp"

namespace
{
void notify_xhci_handler(const Message& m)
{
	kernel::hw::usb::xhci::process_events();
}
} // namespace

namespace kernel::hw
{

void usb_handler_service()
{
	kernel::task::Task* t = kernel::task::CURRENT_TASK;

	pci::initialize();

	usb::xhci::initialize();

	initialize_keyboard();

	t->add_msg_handler(MsgType::NOTIFY_XHCI, notify_xhci_handler);

	kernel::task::process_messages(t);
}

} // namespace kernel::hw
