#include <libs/common/message.hpp>
#include <libs/user/file.hpp>
#include <libs/user/ipc.hpp>
#include <libs/user/keymap.hpp>
#include <libs/user/print.hpp>
#include <libs/user/syscall.hpp>
#include "shell.hpp"
#include "terminal.hpp"

alignas(Terminal) char buffer[sizeof(Terminal)];
alignas(Shell) char shell_buffer[sizeof(Shell)];

int main(void)
{
	auto* s = new (shell_buffer) Shell();
	auto* term = new (buffer) Terminal(s);
	term->print_user();

	// Claim the keyboard focus: the kernel no longer hardwires the shell as
	// the key-event destination (issue #315)
	Message focus = make_request(MsgType::INPUT_SET_FOCUS);
	send_message(process_ids::XHCI, &focus);

	Message msg;
	while (true) {
		// Blocks until a message arrives (issue #314)
		receive_message(&msg);

		switch (msg.type) {
			case MsgType::NOTIFY_KEY_INPUT: {
				// Raw keycode + modifier arrive now; the keymap is shell-side
				// policy (issue #315). Releases are delivered too — ignore.
				if (msg.data.key_input.press == 0) {
					break;
				}
				const char c = keycode_to_ascii(msg.data.key_input.key_code,
												msg.data.key_input.modifier);
				if (c != 0) {
					term->input_char(c);
				}
				break;
			}
			case MsgType::NOTIFY_WRITE:
				if (msg.ool.size != 0) {
					// Large output arrives as a mapped OOL buffer; print()
					// takes any length (printf's buffer is only 512B) and
					// the region must be released afterwards
					const char* text = reinterpret_cast<const char*>(msg.ool.addr);
					term->print(text);
					term->print("\n");
					ool_release(text);
				} else {
					term->printf("%s\n", msg.data.write.buf);
				}
				break;
			case MsgType::NOTIFY_TIMER_TIMEOUT:
				// The only timer this task arms is the cursor-blink one
				// (set_cursor_timer); the kernel no longer attaches a
				// meaning to the expiry (issue #315)
				term->blink_cursor();
				break;
			case MsgType::KERNEL_TASK_READY:
				set_cursor_timer(500);
				break;
			case MsgType::SHELL_COMMAND_DONE: {
				// Sent by Shell::process_input after sys_wait; queued behind
				// the finished command's output, so the prompt reappears
				// only after that output was drawn. A cd child updated this
				// task's working directory on the FS side, so refresh it
				// before showing the prompt.
				char current_dir[13];
				fs_pwd(current_dir, sizeof(current_dir) - 1);
				current_dir[sizeof(current_dir) - 1] = '\0';
				if (current_dir[0] != '\0') {
					term->register_current_dir(current_dir);
				}
				term->enable_input = true;
				term->print_user();
				break;
			}
			default:
				break;
		}
	}

	return 0;
}
