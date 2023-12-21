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

	if (strcmp(command, "ls") == 0) {
		ls(term, "/");
	} else if (strcmp(command, "cat") == 0) {
		cat(term, "KERNEL.ELF");
	} else {
		auto* entry = file_system::find_directory_entry(command, 0);
		if (entry != nullptr) {
			file_system::execute_file(*entry);
			return;
		}

		term.printf("Command not found: %s", command);
	}
}

} // namespace command_line