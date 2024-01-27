#include "syscall.hpp"
#include "../asm_utils.h"
#include "../memory/segment.hpp"
#include "../msr.hpp"
#include <cstdint>

namespace syscall
{

int64_t syscall_handler(uint64_t arg1,
						uint64_t arg2,
						uint64_t arg3,
						uint64_t arg4,
						uint64_t arg5,
						uint64_t arg6)
{
	return 0;
}

void initialize()
{
	// Enable syscall/sysret
	write_msr(IA32_EFER, 0x0501U);

	// Set syscall/sysret entry point
	write_msr(IA32_LSTAR, reinterpret_cast<uint64_t>(syscall_handler));

	// Set syscall/sysret CS and SS
	write_msr(IA32_STAR, (static_cast<uint64_t>(KERNEL_CS) << 32) |
								 (static_cast<uint64_t>(16 | 3) << 48));

	// Set syscall/sysret RFLAGS
	write_msr(IA32_FMASK, 0);
}
} // namespace syscall