#include "command.hpp"
#include "terminal.hpp"

void echo(const char* input, terminal& term)
{
	if (input == nullptr) {
		return term.print("\n");
	}

	term.print(input);
	term.print("\n");
}
