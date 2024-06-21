#pragma once

#include <array>
#include <cstdint>

enum gate_type {
	kTaskGate = 0x5,
	kInterruptGate = 0xE,
	kTrapGate = 0xF,
};

struct type_attr {
	uint16_t interrupt_stack_table : 3;
	uint16_t : 5;
	uint16_t type : 4;
	uint16_t : 1;
	uint16_t dpl : 2;
	uint16_t present : 1;
} __attribute__((packed));

struct idt_entry {
	uint16_t offset_low;
	uint16_t segment_selector;
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

constexpr int IST_FOR_TIMER = 1;
constexpr int IST_FOR_XHCI = 2;
constexpr int IST_FOR_SWITCH_TASK = 3;

void initialize_interrupt();