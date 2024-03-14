#include "shell.hpp"
#include "terminal.hpp"
#include <cstring>

shell::shell() { memset(histories, 0, sizeof(histories)); }

void shell::process_input(char* input, terminal& term)
{
	if (strlen(input) == 0) {
		return;
	}

	term.print(input);
	term.print(" : command not found");
	term.print("\n");
}
