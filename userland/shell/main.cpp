#include "shell.hpp"
#include "terminal.hpp"
#include <../../libs/user/print.hpp>
#include <../../libs/user/syscall.hpp>
#include <cstdlib>
#include <stdalign.h>

alignas(terminal) char buffer[sizeof(terminal)];
alignas(shell) char shell_buffer[sizeof(shell)];

extern "C" int main(int argc, char** argv)
{
	auto* s = new (shell_buffer) shell();
	auto* term = new (buffer) terminal(s);

	term->print_user();

	while (true) {
		char buf[1];
		sys_read(0, buf, 1);
		term->input_char(buf[0]);
	}

	exit(0);
}
