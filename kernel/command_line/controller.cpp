#include "controller.hpp"
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
		term.printf("Command not found: %s", command);
	}
}

} // namespace command_line