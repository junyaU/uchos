#include <libs/common/types.hpp>
#include <libs/user/console.hpp>
#include <libs/user/file.hpp>
#include <libs/user/ipc.hpp>
#include <libs/user/syscall.hpp>

int main(int argc, char** argv)
{
	if (argc < 2) {
		printu("usage: cd <directory>");
		return 0;
	}

	char* target_dir = argv[1];
	char new_path[256] = { 0 };

	fs_change_dir(new_path, target_dir);

	if (new_path[0] == '\0') {
		printu("cd: The directory %s does not exist", target_dir);
		return 0;
	}

	// TODO: ターミナルの改行実装
	printu("");

	return 0;
}
