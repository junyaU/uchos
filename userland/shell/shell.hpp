#pragma once

struct terminal;

struct shell {
	char histories[10][100];

	shell();

	void process_input(char* input, terminal& term);
};
