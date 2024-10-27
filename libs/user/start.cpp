#include <cstdlib>

int main(int argc, char** argv);

extern "C" void _start(int argc, char** argv)
{
	int result = main(argc, argv);
	exit(result);
}
