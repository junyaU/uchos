/**
 * @file memory/segment_utils.h
 * @brief x86-64 segment register and GDT management utilities
 * 
 * This file declares assembly functions for managing segment registers
 * and the Global Descriptor Table (GDT). These functions are essential
 * for setting up and maintaining the segmentation model required by x86-64.
 * 
 * @date 2024
 */

#pragma once

#include <cstdint>

extern "C" {
/**
 * @brief Load the code segment register (CS)
 * 
 * Updates the CS register with the specified segment selector.
 * This defines which segment descriptor is used for code execution.
 * 
 * @param cs Code segment selector value
 * 
 * @note Changing CS requires a far jump or return
 * @warning Incorrect values can cause general protection faults
 */
void load_code_segment(uint16_t cs);

/**
 * @brief Load the stack segment register (SS)
 * 
 * Updates the SS register with the specified segment selector.
 * This defines which segment is used for stack operations.
 * 
 * @param ss Stack segment selector value
 * 
 * @note In 64-bit mode, SS is largely ignored except for privilege checks
 */
void load_stack_segment(uint16_t ss);

/**
 * @brief Load the data segment register (DS)
 * 
 * Updates the DS register with the specified segment selector.
 * This defines the default segment for data accesses.
 * 
 * @param ds Data segment selector value
 * 
 * @note In 64-bit mode, DS is typically set to 0 (null selector)
 */
void load_data_segment(uint16_t ds);

/**
 * @brief Load the Global Descriptor Table (GDT)
 * 
 * Updates the GDTR (GDT Register) with the location and size of the GDT.
 * The GDT contains segment descriptors that define memory segments.
 * 
 * @param size Size of the GDT in bytes minus 1
 * @param offset Linear address of the GDT
 * 
 * @note Uses the LGDT instruction
 * @note The GDT must remain valid in memory after this call
 */
void load_gdt(uint16_t size, uint64_t offset);

/**
 * @brief Load the Task Register (TR)
 * 
 * Updates the TR with a segment selector pointing to a TSS (Task State
 * Segment) descriptor in the GDT. The TSS is used to store information
 * needed for privilege level transitions.
 * 
 * @param tr TSS segment selector value
 * 
 * @note The TSS descriptor must be properly set up in the GDT
 * @note Uses the LTR instruction
 */
void load_tr(uint16_t tr);
}
