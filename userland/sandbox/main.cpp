#include <../../libs/user/ipc.hpp>
#include <../../libs/user/print.hpp>
#include <../../libs/user/syscall.hpp>
#include <cstdio>
#include <cstdlib>

extern "C" int main(void)
{
	int pid = sys_fork();

	if (pid == 0) {
		printf("child process");
	} else {
		printf("parent process, child pid = %d", pid);
	}

	exit(0);
}