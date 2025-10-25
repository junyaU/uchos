/**
 * @file asm_utils.h
 * @brief Assembly utility functions for low-level hardware access
 *
 * This file declares utility functions implemented in assembly that provide
 * low-level access to CPU features, I/O ports, and processor state management.
 * These functions are essential for kernel operations that require direct
 * hardware manipulation.
 *
 * @date 2024
 */

#pragma once

#include <stdint.h>

extern "C" {
/**
 * @brief Read a 32-bit value from an I/O port
 *
 * Performs an x86 IN instruction to read data from the specified I/O port.
 * Used for communicating with legacy hardware devices.
 *
 * @param port The I/O port number to read from
 * @return The 32-bit value read from the port
 *
 * @note This function uses the INL instruction
 */
uint32_t read_from_io_port(uint16_t port);

/**
 * @brief Write a 32-bit value to an I/O port
 *
 * Performs an x86 OUT instruction to write data to the specified I/O port.
 * Used for configuring and controlling legacy hardware devices.
 *
 * @param port The I/O port number to write to
 * @param value The 32-bit value to write
 *
 * @note This function uses the OUTL instruction
 */
void write_to_io_port(uint16_t port, uint32_t value);

/**
 * @brief Find the position of the most significant bit
 *
 * Returns the bit position of the highest set bit in the value.
 * For example, for value 0x80 (10000000b), returns 7.
 *
 * @param value The value to analyze
 * @return Position of the most significant bit (0-31), or -1 if value is 0
 *
 * @note This function uses the BSR (Bit Scan Reverse) instruction
 */
int most_significant_bit(uint32_t value);

/**
 * @brief Write to a Model Specific Register (MSR)
 *
 * Writes a 64-bit value to the specified MSR. MSRs are used to control
 * CPU features and access CPU-specific information.
 *
 * @param msr The MSR number to write to
 * @param value The 64-bit value to write
 *
 * @note Requires CPL 0 (kernel privilege level)
 * @note Uses the WRMSR instruction
 */
void write_msr(uint32_t msr, uint64_t value);

/**
 * @brief Transition from kernel mode to user mode
 *
 * Switches the CPU from kernel mode to user mode to begin executing
 * user space code. Sets up the user stack with arguments and performs
 * the privilege level transition.
 *
 * @param argc Number of command line arguments
 * @param argv Array of command line argument strings
 * @param ss User mode stack segment selector
 * @param rip User mode instruction pointer (entry point)
 * @param rsp User mode stack pointer
 * @param kernel_rsp Pointer to save the kernel stack pointer
 *
 * @note This function does not return normally
 * @note Uses IRETQ to perform the mode switch
 */
void enter_user_mode(int argc,
					 char** argv,
					 uint16_t ss,
					 uint64_t rip,
					 uint64_t rsp,
					 uint64_t* kernel_rsp);

/**
 * @brief Return from user mode to kernel mode
 *
 * Restores the kernel stack and returns to kernel mode execution.
 * This is typically called when a user process exits.
 *
 * @param kernel_rsp The saved kernel stack pointer to restore
 * @param status Exit status code from the user process
 *
 * @note This function restores the kernel context and continues execution
 */
void exit_userland(uint64_t kernel_rsp, int32_t status);
}
