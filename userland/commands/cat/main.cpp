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
	int result = fs_read(fd, buf, sizeof(buf) - 1);  // Leave room for null terminator
	if (result <= 0) {
		// fs_close(fd);
		return 0;
	}

	printu("result: %d", result);

	buf[result] = '\0';  // Add null terminator
	printu(buf);

	// fs_close(fd);

	return 0;
}
