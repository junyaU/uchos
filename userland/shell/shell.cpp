#include "shell.hpp"
#include "command.hpp"
#include "terminal.hpp"
#include <cstring>

shell::shell() { memset(histories, 0, sizeof(histories)); }

void shell::process_input(char* input, terminal& term)
{
	if (strlen(input) == 0) {
		return;
	}

	char* args = strchr(input, ' ');
	int command_length = strlen(input);
	if (args != nullptr) {
		command_length = args - input;
		++args;
	}
	char command_name[command_length + 1];
	memcpy(command_name, input, command_length);
	command_name[command_length] = '\0';

	if (strcmp(command_name, "echo") == 0) {
		return echo(args, term);
	}

	if (strcmp(command_name, "ls") == 0) {
		return ls(args, term);
	}

	if (strcmp(command_name, "cat") == 0) {
		return cat(args, term);
	}

	term.printf("%s : command not found\n", input);
}
