#include "shell.hpp"
#include "command.hpp"
#include "libs/common/types.hpp"
#include "terminal.hpp"
#include <cstring>
#include <libs/user/syscall.hpp>

shell::shell() { memset(histories, 0, sizeof(histories)); }

void shell::process_input(char* input, terminal& term)
{
	if (strlen(input) == 0) {
		term.enable_input = true;
		return;
	}

	const char* args = strchr(input, ' ');
	int command_length = strlen(input);
	if (args != nullptr) {
		command_length = args - input;
		++args;
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
		term.enable_input = true;
		return;
	}

	term.enable_input = false;
}
