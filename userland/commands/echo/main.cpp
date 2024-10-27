#include <cstring>
#include <libs/common/types.hpp>
#include <libs/user/console.hpp>
#include <libs/user/ipc.hpp>
#include <libs/user/syscall.hpp>

int main(int argc, char** argv)
{
	char buf[256];
	if (argv[1] != nullptr) {
		memcpy(buf, argv[1], strlen(argv[1]));
	}

	printu(buf);

	return 0;
}
