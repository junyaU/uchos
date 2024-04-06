#pragma once

#include "graphics/log.hpp"
#include "handlers.hpp"
#include "memory/paging.hpp"
#include "memory/paging_utils.h"

#include <cstdint>

enum exception_code {
	DIVIDE_ERROR = 0,
	DEBUG = 1,
	NMI = 2,
	BREAKPOINT = 3,
	OVERFLOW = 4,
	BOUND_RANGE_EXCEEDED = 5,
	INVALID_OPCODE = 6,
	DEVICE_NOT_AVAILABLE = 7,
	DOUBLE_FAULT = 8,
	COPROCESSOR_SEGMENT_OVERRUN = 9,
	INVALID_TSS = 10,
	SEGMENT_NOT_PRESENT = 11,
	STACK_SEGMENT_FAULT = 12,
	GENERAL_PROTECTION = 13,
	PAGE_FAULT = 14,
	RESERVED = 15,
	X87_FPU_ERROR = 16,
	ALIGNMENT_CHECK = 17,
	MACHINE_CHECK = 18,
	SIMD_FP_EXCEPTION = 19,
	VIRTUALIZATION_EXCEPTION = 20,
};

__attribute__((no_caller_saved_registers)) void
get_fault_name(uint64_t code, char* buf);

template<uint64_t error_code, bool has_error_code>
struct fault_handler;

// no error code
template<uint64_t error_code>
struct fault_handler<error_code, false> {
	static inline __attribute__((interrupt)) void handler(interrupt_frame* frame)
	{
		printk(KERN_ERROR, "Unhandled exception: %d", error_code);
		while (true) {
			__asm__("hlt");
		}
	}
};

// exisiting error code
template<uint64_t error_code>
struct fault_handler<error_code, true> {
	static inline __attribute__((interrupt)) void
	handler(interrupt_frame* frame, uint64_t code)
	{
		if (error_code == PAGE_FAULT) {
			uint64_t fault_addr = get_cr2();
			if (auto err = handle_page_fault(code, fault_addr); !IS_ERR(err)) {
				return;
			}

			printk(KERN_ERROR, "Page fault at %016lx", fault_addr);
		}

		char buf[30];
		get_fault_name(error_code, buf);
		printk(KERN_ERROR, buf);
		kill_userland(frame);
		printk(KERN_ERROR, "RIP: %016lx", frame->rip);
		printk(KERN_ERROR, "RSP: %016lx", frame->rsp);
		printk(KERN_ERROR, "RFLAGS: %016lx", frame->rflags);
		printk(KERN_ERROR, "CS: %016lx", frame->cs);
		printk(KERN_ERROR, "SS: %016lx", frame->ss);

		while (true) {
			__asm__("hlt");
		}
	}
};
