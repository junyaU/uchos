#include <errno.h>
#include <stdint.h>
#include <sys/stat.h>
#include <sys/types.h>

void _exit(int status)
{
	while (1) {
		__asm__("hlt");
	};
}

/*
 * Newlib's malloc arena backs every std container node and printf buffer in
 * the kernel, but tasks are preempted mid-kernel, so two tasks can interleave
 * inside malloc/free and corrupt the arena (issue #383). Newlib serializes
 * through these hooks (no-ops by default); on this single-processor kernel
 * masking interrupts is the lock. The hooks may nest (newlib documents
 * recursive acquisition), hence the depth counter.
 */
static int malloc_lock_depth;
static int malloc_lock_if_was_set;

void __malloc_lock(struct _reent* reent)
{
	uint64_t rflags;
	__asm__ volatile("pushfq\n\tpopq %0\n\tcli" : "=r"(rflags) : : "memory");
	if (malloc_lock_depth++ == 0) {
		malloc_lock_if_was_set = (rflags & 0x200) != 0;
	}
}

void __malloc_unlock(struct _reent* reent)
{
	if (--malloc_lock_depth == 0 && malloc_lock_if_was_set) {
		__asm__ volatile("sti" : : : "memory");
	}
}

caddr_t program_break, program_break_end;

caddr_t sbrk(int incr)
{
	if (program_break == 0 || program_break + incr >= program_break_end) {
		errno = ENOMEM;
		return (caddr_t)-1;
	}

	caddr_t prev_break = program_break;
	program_break += incr;
	return prev_break;
}

int getpid(void) { return 1; }

int kill(int pid, int sig)
{
	errno = EINVAL;
	return -1;
}

int close(int fd)
{
	errno = EBADF;
	return -1;
}

off_t lseek(int fd, off_t offset, int whence)
{
	errno = EBADF;
	return -1;
}

int open(const char* path, int flags)
{
	errno = ENOENT;
	return -1;
}

ssize_t read(int fd, void* buf, size_t count)
{
	errno = EBADF;
	return -1;
}

ssize_t write(int fd, const void* buf, size_t count)
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
