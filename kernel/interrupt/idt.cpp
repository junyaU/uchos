#include "idt.hpp"

#include "handler.hpp"
#include "vector.hpp"

std::array<IDTEntry, 256> idt;

void SetIDTEntry(IDTEntry& entry, uint64_t offset, TypeAttr type_attr,
                 uint16_t segment_selector) {
    entry.offset_low = offset & 0xffffu;
    entry.offset_middle = (offset >> 16) & 0xffffu;
    entry.offset_high = offset >> 32;
    entry.segment_selector = segment_selector;
    entry.ist = 0;
    entry.type_attr = type_attr;
}

void LoadIDT(size_t size, uint64_t addr) {
    IDTR idtr;
    idtr.limit = size - 1;
    idtr.base = addr;
    __asm__("lidt %0" : : "m"(idtr));
}

uint16_t GetCS() {
    uint16_t cs;
    __asm__("mov %%cs, %0" : "=r"(cs));
    return cs;
}

void InitializeInterrupt() {
    auto cs = GetCS();

    SetIDTEntry(idt[InterruptVector::kTest],
                reinterpret_cast<uint64_t>(TestInterrupt),
                TypeAttr{GateType::kInterruptGate, 0, 1}, cs);
    SetIDTEntry(idt[InterruptVector::kLocalApicTimer],
                reinterpret_cast<uint64_t>(TimerInterrupt),
                TypeAttr{GateType::kInterruptGate, 0, 1}, cs);
    LoadIDT(sizeof(idt), reinterpret_cast<uint64_t>(&idt[0]));

    __asm__("sti");
}