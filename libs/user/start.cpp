#include <cstdlib>
#include <libs/user/ipc.hpp>

int main(int argc, char** argv);

extern "C" void _start(int argc, char** argv)
{
	initialize_task();
	int result = main(argc, argv);
	exit(result);
}
