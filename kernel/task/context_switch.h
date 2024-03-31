#pragma once

extern "C" {
void ExecuteContextSwitch(void* next_context, void* current_context);
void restore_context(void* context);
void get_current_context(void* context);
}