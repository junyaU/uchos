#include "idt.hpp"
#include <array>
#include <cstddef>
#include <cstdint>
#include "fault.hpp"
#include "handler.h"
#include "handlers.hpp"
#include "log/log.hpp"
#include "memory/segment.hpp"
#include "vector.hpp"

namespace kernel::interrupt
{

std::array<IdtEntry, 256> idt;

void set_idt_entry(IdtEntry& entry,
				   uint64_t offset,
				   TypeAttr attr,
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
	Idtr r;
	r.limit = size - 1;
	r.base = addr;
	__asm__("lidt %0" : : "m"(r));
}

void initialize_interrupt()
{
	LOG_INFO("Initializing interrupt...");

	auto set_entry = [](int irq, auto handler, uint16_t ist = 0) {
		set_idt_entry(idt[irq], reinterpret_cast<uint64_t>(handler),
					  TypeAttr{ ist, GateType::INTERRUPT_GATE, 0, 1 },
					  kernel::memory::KERNEL_CS);
	};

	set_entry(InterruptVector::LOCAL_APIC_TIMER, on_timer_interrupt, IST_FOR_TIMER);
	set_entry(InterruptVector::XHCI, on_xhci_interrupt, IST_FOR_XHCI);
	set_entry(InterruptVector::VIRTIO_BLK, on_virtio_blk_interrupt);
	set_entry(InterruptVector::VIRTQUEUE_BLK, on_virtio_blk_queue_interrupt);
	set_entry(InterruptVector::VIRTIO_NET, on_virtio_net_interrupt);
	set_entry(InterruptVector::VIRTQUEUE_NET_RX, on_virtio_net_rx_queue_interrupt);
	set_entry(InterruptVector::VIRTQUEUE_NET_TX, on_virtio_net_tx_queue_interrupt);
	set_entry(InterruptVector::SWITCH_TASK, interrupt_task_switch,
			  IST_FOR_SWITCH_TASK);
	set_entry(DIVIDE_ERROR, FaultHandler<DIVIDE_ERROR, false>::handler);
	set_entry(DEBUG, FaultHandler<DEBUG, false>::handler);
	set_entry(BREAKPOINT, FaultHandler<BREAKPOINT, false>::handler);
	set_entry(OVERFLOW, FaultHandler<OVERFLOW, false>::handler);
	set_entry(BOUND_RANGE_EXCEEDED,
			  FaultHandler<BOUND_RANGE_EXCEEDED, false>::handler);
	set_entry(INVALID_OPCODE, FaultHandler<INVALID_OPCODE, false>::handler);
	set_entry(DEVICE_NOT_AVAILABLE,
			  FaultHandler<DEVICE_NOT_AVAILABLE, false>::handler);
	set_entry(DOUBLE_FAULT, FaultHandler<DOUBLE_FAULT, true>::handler);
	set_entry(INVALID_TSS, FaultHandler<INVALID_TSS, true>::handler);
	set_entry(SEGMENT_NOT_PRESENT, FaultHandler<SEGMENT_NOT_PRESENT, true>::handler);
	set_entry(STACK_SEGMENT_FAULT, FaultHandler<STACK_SEGMENT_FAULT, true>::handler);
	set_entry(GENERAL_PROTECTION, FaultHandler<GENERAL_PROTECTION, true>::handler);
	set_entry(PAGE_FAULT, FaultHandler<PAGE_FAULT, true>::handler);
	set_entry(RESERVED, FaultHandler<RESERVED, false>::handler);
	set_entry(X87_FPU_ERROR, FaultHandler<X87_FPU_ERROR, false>::handler);
	set_entry(ALIGNMENT_CHECK, FaultHandler<ALIGNMENT_CHECK, true>::handler);
	set_entry(MACHINE_CHECK, FaultHandler<MACHINE_CHECK, false>::handler);
	set_entry(SIMD_FP_EXCEPTION, FaultHandler<SIMD_FP_EXCEPTION, false>::handler);
	set_entry(VIRTUALIZATION_EXCEPTION,
			  FaultHandler<VIRTUALIZATION_EXCEPTION, false>::handler);

	load_idt(sizeof(idt), reinterpret_cast<uint64_t>(idt.data()));

	__asm__("sti");

	LOG_INFO("Interrupt initialized successfully.");
}

} // namespace kernel::interrupt
