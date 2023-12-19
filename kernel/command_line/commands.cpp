#include "commands.hpp"
#include "graphics/terminal.hpp"

namespace command_line
{
void ls(terminal& term, const char* path) { term.print("ls"); }
} // namespace command_line