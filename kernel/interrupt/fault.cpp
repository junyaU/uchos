#include "fault.hpp"

void get_fault_name(uint64_t code, char* buf)
{
	switch (code) {
		case DIVIDE_ERROR:
			strcpy(buf, "Divide Error");
			break;
		case DEBUG:
			strcpy(buf, "Debug");
			break;
		case NMI:
			strcpy(buf, "Non-Maskable Interrupt");
			break;
		case BREAKPOINT:
			strcpy(buf, "Breakpoint");
			break;
		case OVERFLOW:
			strcpy(buf, "Overflow");
			break;
		case BOUND_RANGE_EXCEEDED:
			strcpy(buf, "Bound Range Exceeded");
			break;
		case INVALID_OPCODE:
			strcpy(buf, "Invalid Opcode");
			break;
		case DEVICE_NOT_AVAILABLE:
			strcpy(buf, "Device Not Available");
			break;
		case DOUBLE_FAULT:
			strcpy(buf, "Double Fault");
			break;
		case COPROCESSOR_SEGMENT_OVERRUN:
			strcpy(buf, "Coprocessor Segment Overrun");
			break;
		case INVALID_TSS:
			strcpy(buf, "Invalid TSS");
			break;
		case SEGMENT_NOT_PRESENT:
			strcpy(buf, "Segment Not Present");
			break;
		case STACK_SEGMENT_FAULT:
			strcpy(buf, "Stack Segment Fault");
			break;
		case GENERAL_PROTECTION:
			strcpy(buf, "General Protection");
			break;
		case PAGE_FAULT:
			strcpy(buf, "Page Fault");
			break;
		case RESERVED:
			strcpy(buf, "Reserved");
			break;
		case X87_FPU_ERROR:
			strcpy(buf, "x87 FPU Error");
			break;
		case ALIGNMENT_CHECK:
			strcpy(buf, "Alignment Check");
			break;
		case MACHINE_CHECK:
			strcpy(buf, "Machine Check");
			break;
		case SIMD_FP_EXCEPTION:
			strcpy(buf, "SIMD Floating-Point Exception");
			break;
		case VIRTUALIZATION_EXCEPTION:
			strcpy(buf, "Virtualization Exception");
			break;
		default:
			strcpy(buf, "Unknown Exception");
			break;
	}
}
