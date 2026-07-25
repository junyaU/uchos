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
	if (IS_ERR(fd)) {
		printu("cat: No such file or directory");
		return 0;
	}

	char buf[1024];
	// Leave room for the null terminator
	ssize_t result = fs_read(fd, buf, sizeof(buf) - 1);
	if (IS_ERR(result)) {
		printu("cat: failed to read %s (%d)", input, static_cast<int>(result));
		return 0;
	}

	buf[result] = '\0'; // Add null terminator (an empty file prints nothing)
	printu(buf);

	return 0;
}
