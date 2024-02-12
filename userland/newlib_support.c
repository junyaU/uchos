#include "syscall.hpp"
#include <errno.h>
#include <stdint.h>
#include <sys/stat.h>
#include <sys/types.h>

int close(int fd)
{
	errno = EBADF;
	return -1;
}

int fstat(int fd, struct stat* buf)
{
	errno = EBADF;
	return -1;
}

int isatty(int fd)
{
	errno = EBADF;
	return -1;
}

off_t lseek(int fd, off_t offset, int whence)
{
	errno = EBADF;
	return -1;
}

ssize_t read(int fd, void* buf, size_t count)
{
	errno = EBADF;
	return -1;
}

caddr_t sbrk(int incr)
{
	errno = ENOMEM;
	return (caddr_t)-1;
}

ssize_t write(int fd, const void* buf, size_t count)
{
	sys_log_string(fd, (uint64_t)buf, count);
	return count;
}

void _exit(int status) { sys_exit(status); }
