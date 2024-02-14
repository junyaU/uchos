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

class terminal;

namespace shell
{
void ls(terminal& term, const char* path);

void cat(terminal& term, const char* file_name);
} // namespace shell