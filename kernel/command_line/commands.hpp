/*
 * @file command_line/commands.hpp
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

namespace command_line
{
void ls(terminal& term, const char* path);
} // namespace command_line