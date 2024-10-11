#include "libs/user/console.hpp"
#include <cstdlib>
#include <cstring>
#include <libs/common/types.hpp>
#include <libs/user/ipc.hpp>
#include <libs/user/syscall.hpp>

extern "C" int main(int argc, char** argv)
{
	char buf[256];
	if (argv[1] != nullptr) {
		memcpy(buf, argv[1], strlen(argv[1]));
	}

	printu(buf);

	exit(0);
}
