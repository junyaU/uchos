/**
 * @file context.hpp
 * @brief Task context structure for saving complete CPU state
 */

#pragma once

#include <array>
#include <cstdint>

namespace kernel::task
{

/**
 * @brief Complete task context for saving/restoring CPU state
 *
 * This structure holds the complete CPU state including general-purpose
 * registers, segment registers, control registers, and FPU/SSE state.
 * It is used during context switches and interrupt handling to preserve
 * the entire state of a task.
 *
 * @note The packed attribute ensures no padding between members
 */
struct context {
	uint64_t cr3;		///< Control register 3 (page table base address)
	uint64_t rip;		///< Instruction pointer
	uint64_t rflags;	///< CPU flags register
	uint64_t reserved1; ///< Reserved for alignment
	uint64_t cs;		///< Code segment selector
	uint64_t ss;		///< Stack segment selector
	uint64_t fs;		///< FS segment selector (often used for TLS)
	uint64_t gs;		///< GS segment selector (often used for CPU-local data)
	uint64_t rax;		///< General-purpose register (return value)
	uint64_t rbx;		///< General-purpose register (callee-saved)
	uint64_t rcx;		///< General-purpose register (4th argument)
	uint64_t rdx;		///< General-purpose register (3rd argument)
	uint64_t rdi;		///< General-purpose register (1st argument)
	uint64_t rsi;		///< General-purpose register (2nd argument)
	uint64_t rsp;		///< Stack pointer
	uint64_t rbp;		///< Base pointer (frame pointer)
	uint64_t r8;		///< General-purpose register (5th argument)
	uint64_t r9;		///< General-purpose register (6th argument)
	uint64_t r10;		///< General-purpose register (caller-saved)
	uint64_t r11;		///< General-purpose register (caller-saved)
	uint64_t r12;		///< General-purpose register (callee-saved)
	uint64_t r13;		///< General-purpose register (callee-saved)
	uint64_t r14;		///< General-purpose register (callee-saved)
	uint64_t r15;		///< General-purpose register (callee-saved)
	/**
	 * @brief FPU/MMX/SSE/SSE2 state save area
	 *
	 * This 512-byte area is used by the FXSAVE/FXRSTOR instructions
	 * to save and restore the floating-point unit and SIMD state.
	 */
	std::array<uint8_t, 512> fxsave_area;
} __attribute__((packed));

} // namespace kernel::task
