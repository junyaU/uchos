#include "../graphics/terminal.hpp"
#include "../types.hpp"
#include "syscall.hpp"
#include <cerrno>
#include <cstdint>

namespace syscall
{

error_t sys_write(uint64_t arg1, uint64_t arg2, uint64_t arg3)
{
	const auto fd = arg1;
	const auto* buf = reinterpret_cast<const char*>(arg2);
	const auto count = arg3;

	if (count > 1024) {
		return E2BIG;
	}

	if (fd == 1) {
		main_terminal->print(buf);
	}

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
			sys_write(arg1, arg2, arg3);
			break;
		default:
			main_terminal->errorf("Unknown syscall number: %d\n", syscall_number);
			break;
	}

	return 0;
}
} // namespace syscall
