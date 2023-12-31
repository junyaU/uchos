#include "idt.hpp"
#include "../graphics/terminal.hpp"
#include "../memory/segment.hpp"
#include "handler.hpp"
#include "vector.hpp"

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
	entry.ist = 0;
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
	main_terminal->info("Initializing interrupt...");

	auto set_entry = [](int irq, auto handler) {
		set_idt_entry(idt[irq], reinterpret_cast<uint64_t>(handler),
					  type_attr{ gate_type::kInterruptGate, 0, 1 }, KERNEL_CS);
	};

	set_entry(InterruptVector::kLocalApicTimer, TimerInterrupt);
	set_entry(InterruptVector::kXHCI, xhci_interrupt);
	set_entry(0, InterruptHandlerDE);
	set_entry(1, InterruptHandlerDB);
	set_entry(3, InterruptHandlerBP);
	set_entry(4, InterruptHandlerOF);
	set_entry(5, InterruptHandlerBR);
	set_entry(6, InterruptHandlerUD);
	set_entry(7, InterruptHandlerNM);
	set_entry(8, InterruptHandlerDF);
	set_entry(10, InterruptHandlerTS);
	set_entry(11, InterruptHandlerNP);
	set_entry(12, InterruptHandlerSS);
	set_entry(13, InterruptHandlerGP);
	set_entry(14, InterruptHandlerPF);
	set_entry(16, InterruptHandlerMF);
	set_entry(17, InterruptHandlerAC);
	set_entry(18, InterruptHandlerMC);
	set_entry(19, InterruptHandlerXM);
	set_entry(20, InterruptHandlerVE);

	load_idt(sizeof(idt), reinterpret_cast<uint64_t>(idt.data()));

	__asm__("sti");

	main_terminal->info("Interrupt initialized successfully.");
}