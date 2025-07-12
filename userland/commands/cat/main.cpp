#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <libs/common/types.hpp>
#include <libs/user/console.hpp>
#include <libs/user/file.hpp>
#include <libs/user/ipc.hpp>
#include <libs/user/syscall.hpp>

int main(int argc, char** argv)
{
	char* input = argv[1];
	if (input == nullptr) {
		printu("cat: missing file operand");
		return 0;
	}

	fd_t fd = fs_open(input, 0);
	if (fd == -1) {
		printu("cat: No such file or directory");
		return 0;
	}

	char buf[1024];
	fs_read(fd, buf, sizeof(buf));

	printu(buf);

	fs_close(fd);

	return 0;
}
