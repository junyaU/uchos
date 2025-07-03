#include "syscall.hpp"
#include "asm_utils.h"
#include "entry.h"
#include "memory/segment.hpp"
#include "msr.hpp"
#include <cstdint>

namespace kernel::syscall
{
void initialize()
{
	// Enable syscall/sysret
	write_msr(IA32_EFER, 0x0501U);

	// Set syscall/sysret entry point
	write_msr(IA32_LSTAR, reinterpret_cast<uint64_t>(syscall_entry));

	// Set syscall/sysret CS and SS
	write_msr(IA32_STAR, (static_cast<uint64_t>(kernel::memory::KERNEL_CS) << 32) |
								 (static_cast<uint64_t>(16 | 3) << 48));

	// Set syscall/sysret RFLAGS
	write_msr(IA32_FMASK, 0);
}
} // namespace kernel::syscall
