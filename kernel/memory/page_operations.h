#pragma once

#include <stdint.h>

extern "C" {
void SetCR3(uint64_t addr);

uint64_t GetCR3();
}