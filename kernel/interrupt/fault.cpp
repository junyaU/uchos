#include "fault.hpp"
#include <string.h> // NOLINT(misc-include-cleaner) - for strlcpy
#include <cstdint>
#include "graphics/log.hpp"
#include "handlers.hpp"
#include "task/task.hpp"

namespace kernel::interrupt
{

void get_fault_name(uint64_t code, char* buf)
{
	switch (code) {
		case DIVIDE_ERROR:
			strlcpy(buf, "Divide Error", 13); // NOLINT(misc-include-cleaner)
			break;
		case DEBUG:
			strlcpy(buf, "Debug", 6);
			break;
		case NMI:
			strlcpy(buf, "Non-Maskable Interrupt", 23);
			break;
		case BREAKPOINT:
			strlcpy(buf, "Breakpoint", 11);
			break;
		case OVERFLOW:
			strlcpy(buf, "Overflow", 9);
			break;
		case BOUND_RANGE_EXCEEDED:
			strlcpy(buf, "Bound Range Exceeded", 20);
			break;
		case INVALID_OPCODE:
			strlcpy(buf, "Invalid Opcode", 14);
			break;
		case DEVICE_NOT_AVAILABLE:
			strlcpy(buf, "Device Not Available", 20);
			break;
		case DOUBLE_FAULT:
			strlcpy(buf, "Double Fault", 12);
			break;
		case COPROCESSOR_SEGMENT_OVERRUN:
			strlcpy(buf, "Coprocessor Segment Overrun", 27);
			break;
		case INVALID_TSS:
			strlcpy(buf, "Invalid TSS", 11);
			break;
		case SEGMENT_NOT_PRESENT:
			strlcpy(buf, "Segment Not Present", 19);
			break;
		case STACK_SEGMENT_FAULT:
			strlcpy(buf, "Stack Segment Fault", 19);
			break;
		case GENERAL_PROTECTION:
			strlcpy(buf, "General Protection", 18);
			break;
		case PAGE_FAULT:
			strlcpy(buf, "Page Fault", 10);
			break;
		case RESERVED:
			strlcpy(buf, "Reserved", 8);
			break;
		case X87_FPU_ERROR:
			strlcpy(buf, "x87 FPU Error", 13);
			break;
		case ALIGNMENT_CHECK:
			strlcpy(buf, "Alignment Check", 15);
			break;
		case MACHINE_CHECK:
			strlcpy(buf, "Machine Check", 13);
			break;
		case SIMD_FP_EXCEPTION:
			strlcpy(buf, "SIMD Floating-Point Exception", 29);
			break;
		case VIRTUALIZATION_EXCEPTION:
			strlcpy(buf, "Virtualization Exception", 24);
			break;
		default:
			strlcpy(buf, "Unknown Exception", 17);
			break;
	}
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

} // namespace kernel::interrupt
