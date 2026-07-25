#include "syscall.hpp"
#include <errno.h>
#include <stdint.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <libs/common/memory_layout.h>

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
	/* The heap region is mapped by the kernel loader at exec (see
	 * kernel/elf.cpp); this hands it out linearly with bounds checks.
	 * The old static 4 KiB array had none: overrunning it silently
	 * corrupted whatever .bss happened to follow (issue #315). */
	static uintptr_t brk = USER_HEAP_BASE;

	uintptr_t next = brk + (intptr_t)incr;
	if (next < USER_HEAP_BASE || next > USER_HEAP_BASE + USER_HEAP_SIZE) {
		errno = ENOMEM;
		return (caddr_t)-1;
	}

	uintptr_t prev = brk;
	brk = next;
	return (caddr_t)prev;
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
