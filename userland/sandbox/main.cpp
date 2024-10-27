#include <cstdio>
#include <libs/common/message.hpp>
#include <libs/user/ipc.hpp>
#include <libs/user/print.hpp>
#include <libs/user/syscall.hpp>

int main(void)
{
	int pid = sys_fork();

	if (pid == 0) {
		printf("child process");
	} else {
		printf("parent process, child pid = %d", pid);
	}

	return 0;
}