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
	term.enable_input = false;

	message m;
	m.type = IPC_FILE_SYSTEM_OPERATION;
	m.data.fs_operation.operation = FS_OP_LIST;
	if (input == nullptr) {
		memcpy(m.data.fs_operation.path, "/", 2);
	} else {
		memcpy(m.data.fs_operation.path, input, strlen(input));
	}

	send_message(0, &m);
}

void cat(char* input, terminal& term)
{
	if (input == nullptr) {
		term.print("cat: missing file operand\n");
		term.enable_input = true;
		return;
	}

	term.enable_input = false;

	message m;
	m.type = IPC_FILE_SYSTEM_OPERATION;
	m.data.fs_operation.operation = FS_OP_READ;
	memcpy(m.data.fs_operation.path, input, strlen(input));

	send_message(0, &m);
}