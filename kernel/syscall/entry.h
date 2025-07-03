/**
 * @file syscall/entry.h
 * @brief System call entry point declaration
 * 
 * This file declares the low-level system call entry point function
 * that is called when user programs execute the SYSCALL instruction.
 * The actual implementation is in assembly code.
 * 
 * @date 2024
 */

#pragma once

extern "C" {
/**
 * @brief Low-level system call entry point
 * 
 * This function is the entry point for all system calls from user space.
 * It is invoked by the SYSCALL instruction and is responsible for:
 * - Saving user context
 * - Switching to kernel stack
 * - Dispatching to the appropriate system call handler
 * - Restoring user context and returning
 * 
 * @note This function is implemented in assembly (syscall_entry.asm)
 * @note The address of this function is loaded into the IA32_LSTAR MSR
 */
void syscall_entry(void);
}
