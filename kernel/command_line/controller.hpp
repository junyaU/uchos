/*
 * @file command_line/controller.hpp
 *
 * @brief Command line controller
 *
 * This file contains the definition of the `controller` class,
 * which is responsible for handling and processing command line inputs.
 *
 */

#pragma once

#include "../graphics/terminal.hpp"

namespace command_line
{
class controller
{
public:
	controller();

	void process_command(const char* command, terminal& term);

private:
	static const int MAX_HISTORY = 10;
	static const int MAX_HISTORY_LENGTH = 100;

	char histories_[MAX_HISTORY][MAX_HISTORY_LENGTH];
	int history_index{ 0 };
};

} // namespace command_line
