#include "command.hpp"
#include "shell.hpp"
#include "terminal.hpp"
#include <libs/user/ipc.hpp>

void cat(char* input, terminal& term)
{
	if (input == nullptr) {
		term.print("cat: missing file operand\n");
		term.enable_input = true;
		return;
	}

	term.enable_input = false;

	message m;
	m.sender = 4;
	m.type = IPC_FILE_SYSTEM_OPERATION;
	m.data.fs_operation.operation = FS_OP_READ;
	memcpy(m.data.fs_operation.path, input, strlen(input));

	send_message(3, &m);
}