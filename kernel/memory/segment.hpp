#pragma once

#include <cstdint>

enum descriptor_type {
	kUpper8Bytes = 0,
	kLDT = 2,
	kTSSAvailable = 9,
	kTSSBusy = 11,
	kCallGate = 12,
	kReadWrite = 2,
	kExecuteRead = 10,
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

const uint16_t KERNEL_CS = 1 << 3;
const uint16_t KERNEL_SS = 2 << 3;
const uint16_t KERNEL_DS = 0;

void setup_segments();
void initialize_segmentation();