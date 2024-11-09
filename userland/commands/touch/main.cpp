#include <libs/user/console.hpp>
#include <libs/user/file.hpp>

int main(int argc, char** argv)
{
	if (argc < 2) {
		printu("Usage: touch <file>");
		return 0;
	}

	const char* path = argv[1];
	fd_t fd = fs_create(path);
	if (fd < 0) {
		printu("Failed to create file: %s", path);
		return 0;
	}

	fs_close(fd);

	printu("File created: %s", path);

	return 0;
}
