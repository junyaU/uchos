#include <libs/common/message.hpp>
#include <libs/user/file.hpp>
#include <libs/user/ipc.hpp>
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

	Message msg;
	while (true) {
		// Blocks until a message arrives (issue #314)
		receive_message(&msg);

		switch (msg.type) {
			case MsgType::NOTIFY_KEY_INPUT:
				term->input_char(msg.data.key_input.ascii);
				break;
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
				switch (msg.data.timer.action) {
					case TimeoutAction::TERMINAL_CURSOR_BLINK:
						term->blink_cursor();
						break;
					default:
						break;
				};
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
