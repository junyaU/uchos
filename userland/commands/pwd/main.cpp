#include <libs/user/console.hpp>
#include <libs/user/file.hpp>

int main(int argc, char** argv)
{
	char name[20];

	const error_t err = fs_pwd(name, 20);
	if (IS_ERR(err)) {
		printu("pwd: failed (%d)", err);
		return 0;
	}

	printu(name);

	return 0;
}
