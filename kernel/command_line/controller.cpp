#include "controller.hpp"
#include "../file_system/fat.hpp"
#include "../graphics/terminal.hpp"
#include "commands.hpp"
#include <cstring>

namespace command_line
{
controller::controller() { memset(histories_, '\0', sizeof(histories_)); }

void controller::process_command(const char* command, terminal& term)
{
	memcpy(histories_[history_write_index_], command, strlen(command));
	history_write_index_ == MAX_HISTORY - 1 ? history_write_index_
											: history_write_index_++;

	const char* args = strchr(command, ' ');
	int command_length = strlen(command);
	if (args != nullptr) {
		command_length = args - command;
		++args;
	}
	char command_name[command_length + 1];
	memcpy(command_name, command, command_length);
	command_name[command_length] = '\0';

	if (strcmp(command_name, "ls") == 0) {
		ls(term, "/");
		return;
	}

	if (strcmp(command_name, "cat") == 0) {
		cat(term, "KERNEL.ELF");
		return;
	}

	auto* entry = file_system::find_directory_entry(command_name, 0);
	if (entry != nullptr) {
		const int status = file_system::execute_file(*entry, args);
		term.printf("Command exited with status %d", status);
		return;
	}

	term.printf("Command not found: %s", command_name);
}

} // namespace command_line