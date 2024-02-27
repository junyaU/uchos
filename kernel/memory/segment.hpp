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

enum descriptor_type {
	UPPER_8_BYTES = 0,
	LDT = 2,
	TSS_AVAILABLE = 9,
	TSS_BUSY = 11,
	CALL_GATE = 12,
	READ_WRITE = 2,
	EXECUTE_READ = 10,
};

union segment_descriptor {
	uint64_t data;

	struct {
		uint64_t limit_low : 16;
		uint64_t base_low : 16;
		uint64_t base_middle : 8;
		descriptor_type type : 4;
		uint64_t system_segment : 1;
		uint64_t descriptor_privilege_level : 2;
		uint64_t present : 1;
		uint64_t limit_high : 4;
		uint64_t available : 1;
		uint64_t long_mode : 1;
		uint64_t default_operation_size : 1;
		uint64_t granularity : 1;
		uint64_t base_high : 8;
	} bits __attribute__((packed));
} __attribute__((packed));

void set_data_segment(segment_descriptor& desc,
					  descriptor_type type,
					  unsigned int dpl);
void set_code_segment(segment_descriptor& desc,
					  descriptor_type type,
					  unsigned int dpl);

constexpr uint16_t KERNEL_CS = 1 << 3;
constexpr uint16_t KERNEL_SS = 2 << 3;
constexpr uint16_t KERNEL_DS = 0;
constexpr uint16_t USER_CS = 4 << 3 | 3;
constexpr uint16_t USER_SS = 3 << 3 | 3;
constexpr uint16_t TSS = 5 << 3;

void setup_segments();
void initialize_segmentation();
void initialize_tss();