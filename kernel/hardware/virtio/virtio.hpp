#pragma once

#include <libs/common/types.hpp>

// Virtio Status Bits
constexpr int VIRTIO_STATUS_ACKNOWLEDGE = 1;
constexpr int VIRTIO_STATUS_DRIVER = 2;
constexpr int VIRTIO_STATUS_DRIVER_OK = 4;
constexpr int VIRTIO_STATUS_FEATURES_OK = 8;
constexpr int VIRTIO_STATUS_DEVICE_NEEDS_RESET = 64;
constexpr int VIRTIO_STATUS_FAILED = 128;

error_t init_virtio_pci();
