#include "terminal.hpp"
#include <../../libs/user/print.hpp>
#include <../../libs/user/syscall.hpp>
#include <cstdlib>
#include <stdalign.h>

alignas(terminal) char buffer[sizeof(terminal)];

extern "C" int main(int argc, char** argv)
{
	auto* term = new (buffer) terminal();
	term->print_user();

	while (true) {
		char buf[2];
		sys_read(0, buf, 1);
		term->print(buf);
	}

	exit(0);
}
