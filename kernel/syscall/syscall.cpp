#include "syscall.hpp"
#include <cstdint>
#include "asm_utils.h"
#include "entry.h"
#include "memory/segment.hpp"
#include "msr.hpp"

namespace kernel::syscall
{
namespace
{
// EFER bits enabled at boot (see msr.hpp for the full bit list); together
// they read as 0x0501.
constexpr uint64_t EFER_SCE = 1U << 0;	///< System Call Extensions (SYSCALL/SYSRET)
constexpr uint64_t EFER_LME = 1U << 8;	///< Long Mode Enable
constexpr uint64_t EFER_LMA = 1U << 10; ///< Long Mode Active

// On 64-bit SYSRET, the CPU computes CS := (IA32_STAR[63:48] + 16) | 3 and
// SS := (IA32_STAR[63:48] + 8) | 3 (RPL is forced to 3 regardless of the
// bits stored here). Our GDT places USER_SS 8 bytes and USER_CS 16 bytes
// above KERNEL_SS's selector, so KERNEL_SS is exactly the base SYSRET
// expects.
constexpr uint64_t SYSRET_SELECTOR_BASE = kernel::memory::KERNEL_SS;
constexpr uint64_t RPL_USER = 3;
static_assert(SYSRET_SELECTOR_BASE + 8 == kernel::memory::USER_SS - RPL_USER);
static_assert(SYSRET_SELECTOR_BASE + 16 == kernel::memory::USER_CS - RPL_USER);
} // namespace

void initialize()
{
	// Enable syscall/sysret
	write_msr(IA32_EFER, EFER_SCE | EFER_LME | EFER_LMA);

	// Set syscall/sysret entry point
	write_msr(IA32_LSTAR, reinterpret_cast<uint64_t>(syscall_entry));

	// Set syscall/sysret CS and SS
	write_msr(IA32_STAR, (static_cast<uint64_t>(kernel::memory::KERNEL_CS) << 32) |
								 ((SYSRET_SELECTOR_BASE | RPL_USER) << 48));

	// Set syscall/sysret RFLAGS
	write_msr(IA32_FMASK, 0);
}
} // namespace kernel::syscall
