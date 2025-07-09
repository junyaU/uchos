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
	return sys_read(fd, (uint64_t)buf, count); 
}

caddr_t sbrk(int incr)
{
	static uint8_t heap[4096];
	static int index = 0;
	int prev_index = index;
	index += incr;
	return (caddr_t)&heap[prev_index];
}

ssize_t write(int fd, const void* buf, size_t count)
{
	return sys_write(fd, (uint64_t)buf, count);
}

void _exit(int status)
{
	sys_exit(status);
	__builtin_unreachable();
}
