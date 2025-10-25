/*
 * @file memory/segment.hpp
 *
 * @brief segmentation
 *
 * This file contains definitions and functions for managing
 * memory segments, particularly for setting up and handling
 * segment descriptors in a low-level system context.
 *
 */

#pragma once

#include <cstdint>

namespace kernel::memory
{

/**
 * @brief Segment descriptor types
 *
 * Defines the various types of segment descriptors used in x86-64
 * protected mode and long mode. These values are used in the type
 * field of segment descriptors.
 */
enum descriptor_type {
	UPPER_8_BYTES = 0, ///< Upper 8 bytes of a 16-byte descriptor
	LDT = 2,		   ///< Local Descriptor Table
	TSS_AVAILABLE = 9, ///< Available Task State Segment
	TSS_BUSY = 11,	   ///< Busy Task State Segment
	CALL_GATE = 12,	   ///< Call gate descriptor
	READ_WRITE = 2,	   ///< Read/Write data segment
	EXECUTE_READ = 10, ///< Execute/Read code segment
};

/**
 * @brief Segment descriptor structure
 *
 * Represents a segment descriptor in the Global Descriptor Table (GDT)
 * or Local Descriptor Table (LDT). This union allows access to the
 * descriptor both as raw data and as individual bit fields.
 *
 * @note In long mode (64-bit), most segmentation features are disabled,
 *       but segment descriptors are still used for privilege levels
 *       and special segments like TSS.
 */
union segment_descriptor {
	uint64_t data; ///< Raw 64-bit descriptor data

	/**
	 * @brief Bit field representation of segment descriptor
	 */
	struct {
		uint64_t limit_low : 16;				 ///< Segment limit bits 0-15
		uint64_t base_low : 16;					 ///< Base address bits 0-15
		uint64_t base_middle : 8;				 ///< Base address bits 16-23
		descriptor_type type : 4;				 ///< Segment type
		uint64_t system_segment : 1;			 ///< 0=system, 1=code/data
		uint64_t descriptor_privilege_level : 2; ///< DPL (0-3)
		uint64_t present : 1;					 ///< Segment present flag
		uint64_t limit_high : 4;				 ///< Segment limit bits 16-19
		uint64_t available : 1;					 ///< Available for OS use
		uint64_t long_mode : 1;					 ///< Long mode code segment flag
		uint64_t default_operation_size : 1;	 ///< 0=16-bit, 1=32-bit
		uint64_t granularity : 1;				 ///< 0=byte, 1=4KB granularity
		uint64_t base_high : 8;					 ///< Base address bits 24-31
	} bits __attribute__((packed));
} __attribute__((packed));

/**
 * @brief Set up a data segment descriptor
 *
 * @param desc Reference to the descriptor to initialize
 * @param type Descriptor type (should be READ_WRITE for data)
 * @param dpl Descriptor privilege level (0-3)
 */
void set_data_segment(segment_descriptor& desc,
					  descriptor_type type,
					  unsigned int dpl);

/**
 * @brief Set up a code segment descriptor
 *
 * @param desc Reference to the descriptor to initialize
 * @param type Descriptor type (should be EXECUTE_READ for code)
 * @param dpl Descriptor privilege level (0-3)
 */
void set_code_segment(segment_descriptor& desc,
					  descriptor_type type,
					  unsigned int dpl);

/**
 * @brief Segment selector constants
 *
 * These constants define the segment selectors used by the kernel.
 * Format: index << 3 | RPL (Requested Privilege Level)
 * @{
 */
constexpr uint16_t KERNEL_CS = 1 << 3; ///< Kernel code segment selector
constexpr uint16_t KERNEL_SS = 2 << 3; ///< Kernel stack segment selector
constexpr uint16_t KERNEL_DS =
		0; ///< Kernel data segment selector (null in long mode)
constexpr uint16_t USER_CS = 4 << 3 | 3; ///< User code segment selector (RPL=3)
constexpr uint16_t USER_SS = 3 << 3 | 3; ///< User stack segment selector (RPL=3)
constexpr uint16_t TSS = 5 << 3;		 ///< Task State Segment selector
/** @} */

/**
 * @brief Set up segment descriptors in the GDT
 *
 * Configures the Global Descriptor Table with the necessary
 * segment descriptors for kernel and user mode operation.
 */
void setup_segments();

/**
 * @brief Initialize the segmentation subsystem
 *
 * Sets up the GDT and loads the segment registers with
 * appropriate selectors for long mode operation.
 */
void initialize_segmentation();

/**
 * @brief Initialize the Task State Segment
 *
 * Sets up the TSS with kernel stack pointers for privilege
 * level transitions. Required for interrupt and system call
 * handling in long mode.
 */
void initialize_tss();

} // namespace kernel::memory