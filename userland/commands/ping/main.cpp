#include <cstring>
#include <libs/user/console.hpp>
#include "libs/user/time.hpp"

int main(int argc, char** argv)
{
	printu("Sleeping for 2 seconds...");

	printu("Sleeping for 2 seconds...");
	printu("Sleeping for 2 seconds...");
	printu("Sleeping for 2 seconds...");
	printu("Sleeping for 2 seconds...");
	printu("Sleeping for 2 seconds...");
	printu("Sleeping for 2 seconds...");

	sleep(10);

	printu("Awake after 2 seconds");

	return 0;
}
