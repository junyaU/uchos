#include "controller.hpp"
#include "../file_system/fat.hpp"
#include "../graphics/terminal.hpp"
#include "commands.hpp"
#include <cstring>

namespace shell
{
controller::controller() { memset(histories_, '\0', sizeof(histories_)); }

// parse redirect command
file_system::directory_entry* parse_redirect_command(char* command)
{
	const char* redirect = strchr(command, '>');
	if (redirect == nullptr) {
		return nullptr;
	}

	command[redirect - command] = '\0';
	++redirect;

	while (*redirect == 0x20) {
		++redirect;
	}

	if (*redirect == '\0') {
		return nullptr;
	}

	auto* entry = file_system::find_directory_entry_by_path(redirect);
	if (entry == nullptr) {
		entry = file_system::create_file(redirect);
	}

	return entry;
}

void controller::process_command(char* command, terminal& term)
{
	memcpy(histories_[history_write_index_], command, strlen(command));
	history_write_index_ == MAX_HISTORY - 1 ? history_write_index_
											: ++history_write_index_;

	char* args = strchr(command, ' ');
	int command_length = strlen(command);
	if (args != nullptr) {
		command_length = args - command;
		++args;
	}
	char command_name[command_length + 1];
	memcpy(command_name, command, command_length);
	command_name[command_length] = '\0';

	auto* redirect_entry = parse_redirect_command(args);

	if (strcmp(command_name, "ls") == 0) {
		ls(term, args, redirect_entry);
		return;
	}

	if (strcmp(command_name, "cat") == 0) {
		cat(term, args);
		return;
	}

	if (strcmp(command_name, "echo") == 0) {
		echo(term, args, redirect_entry);
		return;
	}

	auto* entry = file_system::find_directory_entry(command_name, 0);
	if (entry != nullptr) {
		file_system::execute_file(*entry, args);
		return;
	}

	term.printf("Command not found: %s", command_name);
}

} // namespace shell