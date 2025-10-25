/**
 * @file msr.hpp
 * @brief Model Specific Registers (MSRs) definitions for x86-64 architecture
 *
 * This file defines constants for various Model Specific Registers (MSRs)
 * used in the x86-64 architecture. MSRs are used to control features such as
 * system calls, task switches, and machine check exceptions. They provide an
 * interface for the kernel and other privileged code to configure and manage
 * low-level processor settings and features.
 *
 * @date 2024
 */

#pragma once

#include <cstdint>

/**
 * @brief Extended Feature Enable Register (EFER)
 *
 * Controls enabling of extended features including:
 * - Long Mode Enable (LME) - bit 8
 * - Long Mode Active (LMA) - bit 10
 * - No-Execute Enable (NXE) - bit 11
 * - System Call Extensions (SCE) - bit 0
 */
static constexpr uint32_t IA32_EFER = 0xC0000080;

/**
 * @brief SYSCALL Target Address Register
 *
 * Contains segment selectors and base addresses for SYSCALL/SYSRET instructions:
 * - Bits 63:48 - Base selector for SYSRET CS and SS
 * - Bits 47:32 - Base selector for SYSCALL CS and SS
 */
static constexpr uint32_t IA32_STAR = 0xC0000081;

/**
 * @brief Long Mode SYSCALL Target Address Register
 *
 * Contains the RIP (instruction pointer) for the target of the SYSCALL
 * instruction in 64-bit mode. The kernel's system call entry point address
 * is loaded from this MSR when SYSCALL is executed.
 */
static constexpr uint32_t IA32_LSTAR = 0xC0000082;

/**
 * @brief SYSCALL Flag Mask Register
 *
 * Contains a mask of RFLAGS bits to clear when SYSCALL is executed.
 * This is used to disable interrupts and other flags during system call entry.
 * The kernel typically sets this to clear IF (Interrupt Flag) and TF (Trap Flag).
 */
static constexpr uint32_t IA32_FMASK = 0xC0000084;