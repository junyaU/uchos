#include "fault.hpp"
#include <string.h> // NOLINT(misc-include-cleaner) - for strlcpy
#include <cstddef>
#include <cstdint>
#include "handlers.hpp"
#include "log/log.hpp"
#include "task/task.hpp"

namespace kernel::interrupt
{

// Indexed by ExceptionCode; must stay in the same order as that enum.
static constexpr const char* FAULT_NAMES[] = {
	"Divide Error",					 // DIVIDE_ERROR
	"Debug",						 // DEBUG
	"Non-Maskable Interrupt",		 // NMI
	"Breakpoint",					 // BREAKPOINT
	"Overflow",						 // OVERFLOW
	"Bound Range Exceeded",			 // BOUND_RANGE_EXCEEDED
	"Invalid Opcode",				 // INVALID_OPCODE
	"Device Not Available",			 // DEVICE_NOT_AVAILABLE
	"Double Fault",					 // DOUBLE_FAULT
	"Coprocessor Segment Overrun",	 // COPROCESSOR_SEGMENT_OVERRUN
	"Invalid TSS",					 // INVALID_TSS
	"Segment Not Present",			 // SEGMENT_NOT_PRESENT
	"Stack Segment Fault",			 // STACK_SEGMENT_FAULT
	"General Protection",			 // GENERAL_PROTECTION
	"Page Fault",					 // PAGE_FAULT
	"Reserved",						 // RESERVED
	"x87 FPU Error",				 // X87_FPU_ERROR
	"Alignment Check",				 // ALIGNMENT_CHECK
	"Machine Check",				 // MACHINE_CHECK
	"SIMD Floating-Point Exception", // SIMD_FP_EXCEPTION
	"Virtualization Exception",		 // VIRTUALIZATION_EXCEPTION
};
static constexpr size_t FAULT_NAMES_COUNT =
		sizeof(FAULT_NAMES) / sizeof(FAULT_NAMES[0]);

// Destination buffer size used by get_fault_name/report_fault below.
static constexpr size_t FAULT_NAME_BUF_SIZE = 30;

void get_fault_name(uint64_t code, char* buf)
{
	const char* name =
			code < FAULT_NAMES_COUNT ? FAULT_NAMES[code] : "Unknown Exception";
	strlcpy(buf, name, FAULT_NAME_BUF_SIZE); // NOLINT(misc-include-cleaner)
}

void log_fault_info(const char* fault_type, InterruptFrame* frame)
{
	kernel::task::Task* t = kernel::task::CURRENT_TASK;
	LOG_ERROR("Error in task %d (%s)", t->id, t->name);
	LOG_ERROR("Fault type: %s", fault_type);
	LOG_ERROR("RIP: 0x%016lx", frame->rip);
	LOG_ERROR("RSP: 0x%016lx", frame->rsp);
	LOG_ERROR("RFLAGS: 0x%016lx", frame->rflags);
	LOG_ERROR("CS: 0x%016lx", frame->cs);
	LOG_ERROR("SS: 0x%016lx", frame->ss);
}

const char* report_fault(uint64_t code, InterruptFrame* frame)
{
	static char buf[FAULT_NAME_BUF_SIZE];
	get_fault_name(code, buf);
	log_fault_info(buf, frame);
	return buf;
}

} // namespace kernel::interrupt
