#pragma once

#include <cstddef>
#include <libs/common/types.hpp>

constexpr int INTEL_VENDOR_ID = 0x8086;
constexpr int AMD_VENDOR_ID = 0x1022;
constexpr int NVIDIA_VENDOR_ID = 0x10de;
constexpr int VIRTUALBOX_VENDOR_ID = 0x80ee;
constexpr int VMWARE_VENDOR_ID = 0x15ad;
constexpr int QEMU_VENDOR_ID = 0x1234;
constexpr int REDHAT_VENDOR_ID = 0x1af4;
constexpr int NEC_VENDOR_ID = 0x1033;

error_t output_target_device(char* buf, size_t len, int vendor_id, int device_id);