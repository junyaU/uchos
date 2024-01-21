#pragma once

#include <stdint.h>

extern "C" {
void ExecuteContextSwitch(void* next_context, void* current_context);
void restore_context(void* context);
}