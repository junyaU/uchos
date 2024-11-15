#include <libs/user/console.hpp>
#include <libs/user/file.hpp>

int main(int argc, char** argv)
{
	char name[20];

	get_cwd(name, 20);

	printu(name);

	return 0;
}
