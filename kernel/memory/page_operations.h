#pragma once

#include <stdint.h>

extern "C" {
void set_cr3(uint64_t addr);

uint64_t get_cr3();
}