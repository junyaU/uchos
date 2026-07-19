#include <libs/common/message.hpp>
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
		// Blocks until a message arrives (issue #314); no NO_TASK polling
		receive_message(&msg);

		switch (msg.type) {
			case MsgType::NOTIFY_KEY_INPUT:
				term->input_char(msg.data.key_input.ascii);
				break;
			case MsgType::NOTIFY_WRITE:
				term->printf("%s\n", msg.data.write_shell.buf);
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
			case MsgType::INITIALIZE_TASK:
				set_cursor_timer(500);
				break;
			// Child exits are collected by sys_wait in Shell::process_input
			// now; the IPC_EXIT_TASK round-trip is gone (issue #314 Stage B)
			default:
				break;
		}
	}

	return 0;
}
