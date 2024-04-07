#include "shell.hpp"
#include "libs/common/types.hpp"
#include "terminal.hpp"
#include <cstring>
#include <libs/user/syscall.hpp>

shell::shell() { memset(histories, 0, sizeof(histories)); }

void shell::process_input(char* input, terminal& term)
{
	term.enable_input = true;

	if (strlen(input) == 0) {
		return;
	}

	const char* args = strchr(input, ' ');
	int command_length = strlen(input);
	if (args != nullptr) {
		command_length = args - input;
		++args;
	} else {
		args = "";
	}

	char command_name[command_length + 1];
	memcpy(command_name, input, command_length);
	command_name[command_length] = '\0';

	task_t pid = sys_fork();
	if (pid == 0) {
		error_t status = sys_exec(command_name, args);
		exit(status);
	}

	int child_status;
	sys_wait(&child_status);

	if (child_status != 0) {
		term.printf("%s : command not found\n", command_name);
		return;
	}

	term.enable_input = false;
}
