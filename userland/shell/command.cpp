#include "command.hpp"
#include "terminal.hpp"
#include <../../libs/user/ipc.hpp>

void echo(const char* input, terminal& term)
{
	if (input == nullptr) {
		return term.print("\n");
	}

	term.print(input);
	term.print("\n");
}

void ls(char* input, terminal& term)
{
	message m;
	m.type = IPC_FILE_SYSTEM_OPERATION;
	m.data.fs_operation.operation = FS_OP_LIST;
	m.data.fs_operation.path = input;

	send_message(0, &m);
}