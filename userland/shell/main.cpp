#include "shell.hpp"
#include "terminal.hpp"
#include <libs/common/message.hpp>
#include <libs/user/ipc.hpp>
#include <libs/user/print.hpp>
#include <libs/user/syscall.hpp>

alignas(terminal) char buffer[sizeof(terminal)];
alignas(shell) char shell_buffer[sizeof(shell)];

int main(void)
{
	auto* s = new (shell_buffer) shell();
	auto* term = new (buffer) terminal(s);
	term->print_user();

	message msg;
	while (true) {
		receive_message(&msg);
		if (msg.type == msg_t::NO_TASK) {
			continue;
		}

		switch (msg.type) {
			case msg_t::NOTIFY_KEY_INPUT:
				term->input_char(msg.data.key_input.ascii);
				break;
			case msg_t::NOTIFY_WRITE:
				term->print_message(msg.data.write_shell.buf, msg.is_end_of_message);
				break;
			case msg_t::NOTIFY_TIMER_TIMEOUT:
				switch (msg.data.timer.action) {
					case timeout_action_t::TERMINAL_CURSOR_BLINK:
						term->blink_cursor();
						break;
					default:
						break;
				};
				break;
			case msg_t::INITIALIZE_TASK:
				set_cursor_timer(500);
				break;
			default:
				break;
		}
	}

	return 0;
}
