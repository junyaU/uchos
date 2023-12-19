#include "controller.hpp"
#include "../graphics/terminal.hpp"
#include "commands.hpp"
#include <cstring>

namespace command_line
{
controller::controller() { memset(histories_, '\0', sizeof(histories_)); }

void controller::process_command(const char* command, terminal& term)
{
	if (strcmp(command, "ls") == 0) {
		ls(term, "/");
	} else {
		term.printf("Command not found: %s", command);
	}
}

} // namespace command_line