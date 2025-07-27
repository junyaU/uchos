#pragma once

struct Terminal;

struct Shell {
	char histories[10][100];

	Shell();

	void process_input(char* input, Terminal& term);
};
