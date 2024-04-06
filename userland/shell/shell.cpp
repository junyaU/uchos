#include "shell.hpp"
#include "command.hpp"
#include "terminal.hpp"
#include <cstring>
#include <libs/user/syscall.hpp>

shell::shell() { memset(histories, 0, sizeof(histories)); }

void shell::process_input(char* input, terminal& term)
{
	if (strlen(input) == 0) {
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

	int pid = sys_fork();
	if (pid == 0) {
		sys_exec(command_name, args);
		term.printf("%s : command not found\n", command_name);
		exit(0);
	}

	// term.printf("%s : command not found\n", input);
}
