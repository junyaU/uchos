#include "libs/common/message.hpp"
#include "libs/user/console.hpp"
#include <cstdlib>
#include <cstring>
#include <libs/common/types.hpp>
#include <libs/user/file.hpp>
#include <libs/user/ipc.hpp>
#include <libs/user/syscall.hpp>

extern "C" int main(int argc, char** argv)
{
	char* input = argv[1];
	if (input == nullptr) {
		printu("cat: missing file operand");
		exit(0);
	}

	fd_t fd = fs_open(input, 0);
	if (fd == -1) {
		printu("cat: %s: No such file or directory", input);
		exit(0);
	}

	// TODO: Implement reading file content

	exit(0);
}
