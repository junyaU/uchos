#include "usb/class_driver/keyboard.hpp"
#include <cstdint>
#include <libs/common/message.hpp>
#include <libs/common/process_id.hpp>
#include <libs/common/types.hpp>
#include "keyboard.hpp"
#include "task/ipc.hpp"
#include "task/task.hpp"

namespace
{
/// Task that raw key events are delivered to. Events are dropped until a
/// task claims the focus with INPUT_SET_FOCUS (issue #315): the kernel no
/// longer hardwires the shell as the destination.
ProcessId focus_task = process_ids::INVALID;

void handle_set_focus(const Message& m) { focus_task = m.sender; }
} // namespace

namespace kernel::hw
{

void initialize_keyboard()
{
	// Registered on the USB handler task before its message loop starts, so
	// a focus request queued during boot is never dispatched unhandled
	kernel::task::CURRENT_TASK->add_msg_handler(MsgType::INPUT_SET_FOCUS,
												handle_set_focus);

	usb::KeyboardDriver::default_observer = [](uint8_t modifier, uint8_t keycode,
											   bool press) {
		if (focus_task == process_ids::INVALID) {
			return;
		}

		// Raw delivery (issue #315): no keymap, no press filtering. What a
		// key means is decided by the focus owner in user space.
		// This observer runs in the USB handler task, not in interrupt
		// context; name the real sender (issue #314 Stage C)
		Message m{ .type = MsgType::NOTIFY_KEY_INPUT, .sender = process_ids::XHCI };
		m.data.key_input.key_code = keycode;
		m.data.key_input.modifier = modifier;
		m.data.key_input.press = static_cast<int>(press);

		kernel::task::send_message(focus_task, m);
	};
}

} // namespace kernel::hw
