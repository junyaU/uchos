#pragma once

#include <array>
#include <cstdint>

struct TypeAttr {
    uint8_t type : 4;
    uint8_t : 1;
    uint8_t dpl : 2;
    uint8_t present : 1;
};

struct IDTEntry {
    uint16_t offset_low;
    uint16_t segment_selector;
    uint8_t ist;
    TypeAttr type_attr;
    uint16_t offset_middle;
    uint32_t offset_high;
    uint32_t zero;
} __attribute__((packed));

extern std::array<IDTEntry, 256> idt;

struct IDTR {
    uint16_t limit;
    uint64_t base;
} __attribute__((packed));

enum GateType {
    kTaskGate = 0x5,
    kInterruptGate = 0xE,
    kTrapGate = 0xF,
};

void InitializeInterrupt();