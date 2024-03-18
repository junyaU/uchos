#pragma once

struct terminal;

extern int SHELL_TASK_ID;

struct shell {
	char histories[10][100];

	shell();

	void process_input(char* input, terminal& term);
};
