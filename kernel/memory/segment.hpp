#pragma once

#include <array>
#include <cstdint>

enum DescriptorType {
    kUpper8Bytes = 0,
    kLDT = 2,
    kTSSAvailable = 9,
    kTSSBusy = 11,
    kCallGate = 12,
    kReadWrite = 2,
    kExecuteRead = 10,
};

union SegmentDescriptor {
    uint64_t data;

    struct {
        uint64_t limit_low : 16;
        uint64_t base_low : 16;
        uint64_t base_middle : 8;
        DescriptorType type : 4;
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

void SetDataSegment(SegmentDescriptor& desc, DescriptorType type,
                    unsigned int descriptor_privilege_level);
void SetCodeSegment(SegmentDescriptor& desc, DescriptorType type,
                    unsigned int descriptor_privilege_level);

const uint16_t kKernelCS = 1 << 3;
const uint16_t kKernelSS = 2 << 3;
const uint16_t kKernelDS = 0;

void SetupSegments();
void InitializeSegmentation();