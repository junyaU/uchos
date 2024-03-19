#include "shell.hpp"
#include "terminal.hpp"
#include <../../libs/user/ipc.hpp>
#include <../../libs/user/print.hpp>
#include <../../libs/user/syscall.hpp>
#include <cstdlib>
#include <stdalign.h>

alignas(terminal) char buffer[sizeof(terminal)];
alignas(shell) char shell_buffer[sizeof(shell)];

extern "C" int main(void)
{
	initialize_task();

	auto* s = new (shell_buffer) shell();
	auto* term = new (buffer) terminal(s);
	term->print_user();

	message msg;
	while (true) {
		receive_message(&msg);
		if (msg.type == NO_TASK) {
			continue;
		}

		switch (msg.type) {
			case NOTIFY_KEY_INPUT:
				term->input_char(msg.data.key_input.ascii);
				break;
			case NOTIFY_WRITE:
				term->print_message(msg.data.write_shell.buf,
									msg.data.write_shell.is_end_of_message);
				break;
			case NOTIFY_CURSOR_BLINK:
				term->blink_cursor();
				break;
			case IPC_INITIALIZE_TASK:
				SHELL_TASK_ID = msg.data.init.task_id;
				break;
			default:
				break;
		}
	}

	exit(0);
}
