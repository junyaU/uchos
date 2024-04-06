#include "idt.hpp"
#include "../graphics/log.hpp"
#include "../memory/segment.hpp"
#include "fault.hpp"
#include "handler.h"
#include "handlers.hpp"
#include "vector.hpp"
#include <array>
#include <libs/common/types.hpp>

std::array<idt_entry, 256> idt;

void set_idt_entry(idt_entry& entry,
				   uint64_t offset,
				   type_attr attr,
				   uint16_t segment_selector)
{
	entry.offset_low = offset & 0xffffU;
	entry.offset_middle = (offset >> 16) & 0xffffU;
	entry.offset_high = offset >> 32;
	entry.segment_selector = segment_selector;
	entry.attr = attr;
}

void load_idt(size_t size, uint64_t addr)
{
	idtr r;
	r.limit = size - 1;
	r.base = addr;
	__asm__("lidt %0" : : "m"(r));
}

void initialize_interrupt()
{
	printk(KERN_INFO, "Initializing interrupt...");

	auto set_entry = [](int irq, auto handler, uint16_t ist = 0) {
		set_idt_entry(idt[irq], reinterpret_cast<uint64_t>(handler),
					  type_attr{ ist, gate_type::kInterruptGate, 0, 1 }, KERNEL_CS);
	};

	set_entry(InterruptVector::kLocalApicTimer, on_timer_interrupt, IST_FOR_TIMER);
	set_entry(InterruptVector::kXHCI, on_xhci_interrupt, IST_FOR_XHCI);
	set_entry(DIVIDE_ERROR, fault_handler<DIVIDE_ERROR, false>::handler);
	set_entry(DEBUG, fault_handler<DEBUG, false>::handler);
	set_entry(BREAKPOINT, fault_handler<BREAKPOINT, false>::handler);
	set_entry(OVERFLOW, fault_handler<OVERFLOW, false>::handler);
	set_entry(BOUND_RANGE_EXCEEDED,
			  fault_handler<BOUND_RANGE_EXCEEDED, false>::handler);
	set_entry(INVALID_OPCODE, fault_handler<INVALID_OPCODE, false>::handler);
	set_entry(DEVICE_NOT_AVAILABLE,
			  fault_handler<DEVICE_NOT_AVAILABLE, false>::handler);
	set_entry(DOUBLE_FAULT, fault_handler<DOUBLE_FAULT, true>::handler);
	set_entry(INVALID_TSS, fault_handler<INVALID_TSS, true>::handler);
	set_entry(SEGMENT_NOT_PRESENT,
			  fault_handler<SEGMENT_NOT_PRESENT, true>::handler);
	set_entry(STACK_SEGMENT_FAULT,
			  fault_handler<STACK_SEGMENT_FAULT, true>::handler);
	set_entry(GENERAL_PROTECTION, fault_handler<GENERAL_PROTECTION, true>::handler);
	set_entry(PAGE_FAULT, fault_handler<PAGE_FAULT, true>::handler);
	set_entry(RESERVED, fault_handler<RESERVED, false>::handler);
	set_entry(X87_FPU_ERROR, fault_handler<X87_FPU_ERROR, false>::handler);
	set_entry(ALIGNMENT_CHECK, fault_handler<ALIGNMENT_CHECK, true>::handler);
	set_entry(MACHINE_CHECK, fault_handler<MACHINE_CHECK, false>::handler);
	set_entry(SIMD_FP_EXCEPTION, fault_handler<SIMD_FP_EXCEPTION, false>::handler);
	set_entry(VIRTUALIZATION_EXCEPTION,
			  fault_handler<VIRTUALIZATION_EXCEPTION, false>::handler);

	load_idt(sizeof(idt), reinterpret_cast<uint64_t>(idt.data()));

	__asm__("sti");

	printk(KERN_INFO, "Interrupt initialized successfully.");
}
