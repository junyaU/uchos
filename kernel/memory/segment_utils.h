#pragma once

#include <cstdint>

extern "C" {
void load_code_segment(uint16_t cs);

void load_stack_segment(uint16_t ss);

void load_data_segment(uint16_t ds);

void load_gdt(uint16_t size, uint64_t offset);

void load_tr(uint16_t tr);
}
