/**
 * @file memory/paging_utils.h
 * @brief Low-level paging and control register utilities
 * 
 * This file declares assembly functions for managing x86-64 paging
 * and control registers. These functions provide direct access to
 * CPU paging mechanisms and are critical for virtual memory management.
 * 
 * @date 2024
 */

#pragma once

#include <cstdint>

extern "C" {
/**
 * @brief Flush a single TLB entry
 * 
 * Invalidates the Translation Lookaside Buffer (TLB) entry for the
 * specified virtual address. This forces the CPU to reload the page
 * table entry on the next access to this address.
 * 
 * @param addr Virtual address whose TLB entry should be flushed
 * 
 * @note Uses the INVLPG instruction
 * @note Only flushes the entry for the current CPU
 */
void flush_tlb(uint64_t addr);

/**
 * @brief Read the CR0 control register
 * 
 * CR0 contains system control flags that control operating mode and
 * states of the processor, including:
 * - PE (Protection Enable)
 * - PG (Paging)
 * - WP (Write Protect)
 * 
 * @return Current value of CR0
 */
uint64_t get_cr0();

/**
 * @brief Write to the CR0 control register
 * 
 * Updates the CR0 register with the specified value. Care must be
 * taken when modifying CR0 as it controls fundamental CPU operation modes.
 * 
 * @param addr New value to write to CR0
 * 
 * @warning Incorrect values can cause system instability
 */
void set_cr0(uint64_t addr);

/**
 * @brief Read the CR2 control register
 * 
 * CR2 contains the linear address that caused the most recent page fault.
 * This is typically read in the page fault handler to determine which
 * address caused the fault.
 * 
 * @return The faulting linear address from the last page fault
 */
uint64_t get_cr2();

/**
 * @brief Write to the CR3 control register
 * 
 * CR3 contains the physical address of the page directory base (PML4 in
 * x86-64). Changing CR3 switches the active page table, effectively
 * changing the virtual address space.
 * 
 * @param addr Physical address of the PML4 table
 * 
 * @note Writing to CR3 flushes the entire TLB (except global pages)
 */
void set_cr3(uint64_t addr);

/**
 * @brief Read the CR3 control register
 * 
 * Returns the current page table base address (PML4 table physical address)
 * and PCID (Process Context Identifier) if supported.
 * 
 * @return Current value of CR3 (page table base + flags)
 */
uint64_t get_cr3();
}
