#pragma once

#include <stdint.h>

extern "C" {
void LoadCodeSegment(uint16_t cs);

void LoadStackSegment(uint16_t ss);

void LoadDataSegment(uint16_t ds);

void LoadGDT(uint16_t size, uint64_t offset);

void SetCR3(uint64_t addr);

uint64_t GetCR3();
}