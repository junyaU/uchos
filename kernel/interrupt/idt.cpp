#include "idt.hpp"
#include "../memory/segment.hpp"
#include "handler.hpp"
#include "vector.hpp"
#include <array>
#include <stddef.h>

std::array<IDTEntry, 256> idt;

void SetIDTEntry(IDTEntry& entry,
				 uint64_t offset,
				 TypeAttr type_attr,
				 uint16_t segment_selector)
{
	entry.offset_low = offset & 0xffffU;
	entry.offset_middle = (offset >> 16) & 0xffffU;
	entry.offset_high = offset >> 32;
	entry.segment_selector = segment_selector;
	entry.ist = 0;
	entry.type_attr = type_attr;
}

void LoadIDT(size_t size, uint64_t addr)
{
	IDTR idtr;
	idtr.limit = size - 1;
	idtr.base = addr;
	__asm__("lidt %0" : : "m"(idtr));
}

uint16_t GetCS()
{
	uint16_t cs;
	__asm__("mov %%cs, %0" : "=r"(cs));
	return cs;
}

void InitializeInterrupt()
{
	auto set_idt_entry = [](int irq, auto handler) {
		SetIDTEntry(idt[irq], reinterpret_cast<uint64_t>(handler),
					TypeAttr{ GateType::kInterruptGate, 0, 1 }, kKernelCS);
	};

	set_idt_entry(InterruptVector::kLocalApicTimer, TimerInterrupt);
	set_idt_entry(0, InterruptHandlerDE);
	set_idt_entry(1, InterruptHandlerDB);
	set_idt_entry(3, InterruptHandlerBP);
	set_idt_entry(4, InterruptHandlerOF);
	set_idt_entry(5, InterruptHandlerBR);
	set_idt_entry(6, InterruptHandlerUD);
	set_idt_entry(7, InterruptHandlerNM);
	set_idt_entry(8, InterruptHandlerDF);
	set_idt_entry(10, InterruptHandlerTS);
	set_idt_entry(11, InterruptHandlerNP);
	set_idt_entry(12, InterruptHandlerSS);
	set_idt_entry(13, InterruptHandlerGP);
	set_idt_entry(14, InterruptHandlerPF);
	set_idt_entry(16, InterruptHandlerMF);
	set_idt_entry(17, InterruptHandlerAC);
	set_idt_entry(18, InterruptHandlerMC);
	set_idt_entry(19, InterruptHandlerXM);
	set_idt_entry(20, InterruptHandlerVE);

	LoadIDT(sizeof(idt), reinterpret_cast<uint64_t>(idt.data()));

	__asm__("sti");
}