#include <cstring>
#include <libs/common/message.hpp>
#include <libs/common/types.hpp>
#include <libs/user/file.hpp>
#include <libs/user/ipc.hpp>
#include <libs/user/keymap.hpp>
#include <libs/user/print.hpp>
#include <libs/user/syscall.hpp>
#include "shell.hpp"
#include "terminal.hpp"

alignas(Terminal) char buffer[sizeof(Terminal)];
alignas(Shell) char shell_buffer[sizeof(Shell)];

namespace
{

// KERNEL_SMOKE_TEST builds exec the shell with "-smoke" (issue #374): run
// one command through the normal process_input fork→exec→wait path and
// report the outcome to the kernel, which prints the CI serial marker and
// exits QEMU. The command's own output still goes to the screen as usual.
void run_smoke_test(Shell* s, Terminal* term)
{
	char command[] = "echo smoke";
	const error_t status = s->process_input(command, *term);

	// One ping to pongd, the first manifest-launched ring-3 service: covers
	// the ELF service boot path and the user-side reply path (issue #315
	// 3b-10). The service answers "pong" — a transformation, so a lost
	// reply leaves the zeroed request buffer behind and cannot pass.
	Message ping = make_request(MsgType::PONGD_PING);
	const Message pong = call(process_ids::PONGD, &ping);
	error_t svc_status = OK;
	if (IS_ERR(pong.result)) {
		svc_status = pong.result;
	} else if (strcmp(pong.data.write.buf, "pong") != 0) {
		svc_status = ERR_INVALID_ARG;
	}

	// Open, read and close one file: open installs the fd entry through the
	// FS's FD_INSTALL request to KERNEL, read follows that entry, close
	// removes it with FD_REMOVE. The echo binary is always on the smoke
	// disk, so its ELF magic is a fixed expected value. close is a one-way
	// send, so reopen to prove it ran: the FS handles requests in order, so
	// a removed entry must free the same lowest slot for the next open.
	error_t file_status = OK;
	const fd_t fd = fs_open("echo", 0);
	if (fd < 0) {
		file_status = fd;
	} else {
		char magic[4] = {};
		const ssize_t n = fs_read(fd, magic, sizeof(magic));
		if (n < 0) {
			file_status = static_cast<error_t>(n);
		} else if (n != sizeof(magic) || memcmp(magic, "\x7f" "ELF", 4) != 0) {
			file_status = ERR_INVALID_ARG;
		}
		fs_close(fd);

		const fd_t reopened = fs_open("echo", 0);
		if (!IS_ERR(file_status) && reopened != fd) {
			file_status = reopened < 0 ? reopened : ERR_INVALID_FD;
		}
		if (reopened >= 0) {
			fs_close(reopened);
		}
	}

	error_t first_error = status;
	if (!IS_ERR(first_error)) {
		first_error = svc_status;
	}
	if (!IS_ERR(first_error)) {
		first_error = file_status;
	}

	Message report = make_request(MsgType::SMOKE_REPORT);
	report.data.smoke.status = first_error;
	send_message(process_ids::KERNEL, &report);
}

} // namespace

int main(int argc, char** argv)
{
	auto* s = new (shell_buffer) Shell();
	auto* term = new (buffer) Terminal(s);
	term->print_user();

	// Claim the keyboard focus: the kernel no longer hardwires the shell as
	// the key-event destination (issue #315)
	Message focus = make_request(MsgType::INPUT_SET_FOCUS);
	send_message(process_ids::XHCI, &focus);

	if (argc > 1 && strcmp(argv[1], "-smoke") == 0) {
		run_smoke_test(s, term);
	}

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
