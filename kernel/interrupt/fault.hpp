#pragma once

#include "graphics/log.hpp"
#include "handlers.hpp"
#include "libs/common/types.hpp"
#include "memory/paging.hpp"
#include "memory/paging_utils.h"
#include "panic.hpp"

#include <cstdint>

namespace kernel::interrupt
{

enum ExceptionCode {
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

__attribute__((no_caller_saved_registers)) void get_fault_name(uint64_t code,
															   char* buf);

__attribute__((no_caller_saved_registers)) void log_fault_info(
		const char* fault_type,
		InterruptFrame* frame);

/**
 * @brief Look up the fault name and log the frame in one call
 *
 * Combines get_fault_name() and log_fault_info(), which every FaultHandler
 * specialization below needs before it panics.
 *
 * @param code Exception code (see ExceptionCode)
 * @param frame Interrupt frame at the time of the fault
 * @return Human-readable fault name (valid until the next call)
 */
__attribute__((no_caller_saved_registers)) const char* report_fault(
		uint64_t code,
		InterruptFrame* frame);

template<uint64_t error_code, bool has_error_code>
struct FaultHandler;

// no error code
template<uint64_t error_code>
struct FaultHandler<error_code, false> {
	static inline __attribute__((interrupt)) void handler(InterruptFrame* frame)
	{
		const char* name = report_fault(error_code, frame);

		PANIC("Fatal exception: %s", name);
	}
};

// exisiting error code
template<uint64_t error_code>
struct FaultHandler<error_code, true> {
	static inline __attribute__((interrupt)) void handler(InterruptFrame* frame,
														  uint64_t code)
	{
		if (error_code == PAGE_FAULT) {
			const uint64_t fault_addr = get_cr2();
			if (auto err = kernel::memory::handle_page_fault(code, fault_addr);
				!IS_ERR(err)) {
				return;
			}

			LOG_ERROR("Page fault at %016lx", fault_addr);
		}

		const char* name = report_fault(error_code, frame);

		kill_userland(frame);

		PANIC("Fatal exception with error code: %s (code: 0x%lx)", name, code);
	}
};

} // namespace kernel::interrupt
