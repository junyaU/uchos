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
				term->printf("%s\n", msg.data.write_shell.buf);
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
			case msg_t::FS_CHANGE_DIR:
				term->register_current_dir(msg.data.fs.name);
				break;
			case msg_t::INITIALIZE_TASK:
				set_cursor_timer(500);
				break;
			case msg_t::IPC_EXIT_TASK:
				term->enable_input = true;
				term->print_user();
			default:
				break;
		}
	}

	return 0;
}
