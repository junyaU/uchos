#pragma once

#include <array>
#include <cstdint>

struct type_attr {
	uint8_t type : 4;
	uint8_t : 1;
	uint8_t dpl : 2;
	uint8_t present : 1;
};

struct idt_entry {
	uint16_t offset_low;
	uint16_t segment_selector;
	uint8_t ist;
	type_attr attr;
	uint16_t offset_middle;
	uint32_t offset_high;
	uint32_t zero;
} __attribute__((packed));

extern std::array<idt_entry, 256> idt;

struct idtr {
	uint16_t limit;
	uint64_t base;
} __attribute__((packed));

enum gate_type {
	kTaskGate = 0x5,
	kInterruptGate = 0xE,
	kTrapGate = 0xF,
};

void initialize_interrupt();