/*
 * @file shell/commands.hpp
 *
 * @brief Command line commands
 *
 * This file contains the declarations of command line functions.
 * Currently, these functions are implemented in the kernel space as a temporary
 * measure until a more robust system call mechanism is developed for user space.
 *
 */

#pragma once

#include "../file_system/fat.hpp"

class terminal;

namespace shell
{
void ls(terminal& term,
		const char* path,
		file_system::directory_entry* redirect_entry);

void cat(terminal& term, const char* file_name);

void echo(terminal& term,
		  const char* s,
		  file_system::directory_entry* redirect_entry);
} // namespace shell