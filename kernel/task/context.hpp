#pragma once

#include <array>
#include <cstdint>

struct Context {
	uint64_t cr3, rip, rflags, reserved1;
	uint64_t cs, ss, fs, gs;
	uint64_t rax, rbx, rcx, rdx, rdi, rsi, rsp, rbp;
	uint64_t r8, r9, r10, r11, r12, r13, r14, r15;
	std::array<uint8_t, 512> fxsave_area;
} __attribute__((packed));

extern Context task_2_context, task_main_context alignas(16);
