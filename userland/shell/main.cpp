#include "terminal.hpp"
#include <../../libs/user/print.hpp>
#include <cstdio>
#include <cstdlib>

extern "C" int main(int argc, char** argv)
{
	auto term = terminal();
	term.print(term.user_name);
	term.print(" Hello, uchos!");

	exit(0);
}
