#pragma once

extern "C" {
void flash_tlb(uint64_t addr);

void set_cr3(uint64_t addr);

uint64_t get_cr3();
}