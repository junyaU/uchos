#pragma once

#include <libs/common/types.hpp>

struct Terminal;

struct Shell {
	char histories[10][100];

	Shell();

	/**
	 * @brief Run one command line through fork→exec→wait
	 * @param input Command line to execute (mutated during parsing)
	 * @param term Terminal for prompt and error output
	 * @return OK when the child ran and exited with status 0; the fork
	 * error or the child's non-zero exit status otherwise
	 */
	error_t process_input(char* input, Terminal& term);
};
