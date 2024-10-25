#include "libs/user/console.hpp"
#include <cstddef>
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
		printu("cat: No such file or directory");
		exit(0);
	}

	char buf[1024];
	size_t result = fs_read(fd, buf, sizeof(buf));
	if (result == 0) {
		printu("cat: cannot read file");
		exit(0);
	}

	printu(buf);

	exit(0);
}
