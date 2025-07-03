/**
 * @file task/context_switch.h
 * @brief Low-level context switching functions
 * 
 * This file declares the assembly-implemented functions for saving and
 * restoring CPU contexts during task switches. These functions handle
 * the low-level details of preserving and switching between different
 * execution contexts in the kernel.
 * 
 * @date 2024
 */

#pragma once

extern "C" {
/**
 * @brief Perform a context switch between two tasks
 * 
 * Saves the current CPU state to current_context and restores the
 * CPU state from next_context. This effectively switches execution
 * from one task to another.
 * 
 * @param next_context Pointer to the context to switch to
 * @param current_context Pointer to save the current context
 * 
 * @note Both contexts must point to valid task_context structures
 * @note This function is implemented in assembly (context_switch.asm)
 */
void ExecuteContextSwitch(void* next_context, void* current_context);

/**
 * @brief Restore a saved context and resume execution
 * 
 * Loads the CPU state from the given context and jumps to the
 * saved instruction pointer. This is used to start new tasks or
 * resume previously saved contexts.
 * 
 * @param context Pointer to the context to restore
 * 
 * @note This function does not return to the caller
 * @note This function is implemented in assembly (context_switch.asm)
 */
void restore_context(void* context);

/**
 * @brief Save the current CPU context
 * 
 * Captures the current CPU state including all general-purpose
 * registers, stack pointer, and instruction pointer into the
 * provided context structure.
 * 
 * @param context Pointer to store the current context
 * 
 * @note The context must point to a valid task_context structure
 * @note This function is implemented in assembly (context_switch.asm)
 */
void get_current_context(void* context);
}