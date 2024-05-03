#pragma once

#include <libs/common/types.hpp>

// Virtio Status Bits
constexpr int VIRTIO_STATUS_ACKNOWLEDGE = 1;
constexpr int VIRTIO_STATUS_DRIVER = 2;
constexpr int VIRTIO_STATUS_DRIVER_OK = 4;
constexpr int VIRTIO_STATUS_FEATURES_OK = 8;
constexpr int VIRTIO_STATUS_DEVICE_NEEDS_RESET = 64;
constexpr int VIRTIO_STATUS_FAILED = 128;

// Virtio Device Features
constexpr int VIRTIO_F_INDIRECT_DESC = 28;
constexpr int VIRTIO_F_EVENT_IDX = 29;
constexpr int VIRTIO_F_VERSION_1 = 32;
constexpr int VIRTIO_F_ACCESS_PLATFORM = 33;
constexpr int VIRTIO_F_RING_PACKED = 34;
constexpr int VIRTIO_F_IN_ORDER = 35;
constexpr int VIRTIO_F_ORDER_PLATFORM = 36;
constexpr int VIRTIO_F_SR_IOV = 37;
constexpr int VIRTIO_F_NOTIFICATION_DATA = 38;
constexpr int VIRTIO_F_NOTIF_CONFIG_DATA = 39;
constexpr int VIRTIO_F_RING_RESET = 40;

error_t init_virtio_pci();
