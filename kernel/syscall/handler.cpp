#include "../graphics/terminal.hpp"
#include "../types.hpp"
#include "syscall.hpp"
#include <cstdint>

namespace syscall
{

error_t sys_write(uint64_t arg1)
{
	const char* s = reinterpret_cast<const char*>(arg1);
	main_terminal->printf("%s\n", s);

	return OK;
}

extern "C" int64_t handle_syscall(uint64_t arg1,
								  uint64_t arg2,
								  uint64_t arg3,
								  uint64_t arg4,
								  uint64_t arg5,
								  uint64_t syscall_number)
{
	switch (syscall_number) {
		case SYS_WRITE:
			sys_write(arg1);
			break;
		default:
			main_terminal->errorf("Unknown syscall number: %d\n", syscall_number);
			break;
	}

	return 0;
}
} // namespace syscall
