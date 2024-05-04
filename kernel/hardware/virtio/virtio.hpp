#pragma once

#include <cstdint>
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

// Virtqueue Flags
/* This marks a buffer as continuing via the next field. */
constexpr int VIRTQ_DESC_F_NEXT = 1;
/* This marks a buffer as write-only (otherwise read-only). */
constexpr int VIRTQ_DESC_F_WRITE = 2;
/* This means the buffer contains a list of buffer descriptors. */
constexpr int VIRTQ_DESC_F_INDIRECT = 4;

// https://docs.oasis-open.org/virtio/virtio/v1.2/csd01/virtio-v1.2-csd01.html#x1-430005
struct virtq_desc {
	uint64_t addr;
	uint32_t len;
	uint16_t flags;
	uint16_t next;
} __attribute__((packed));

// https://docs.oasis-open.org/virtio/virtio/v1.2/csd01/virtio-v1.2-csd01.html#x1-490006
struct virtq_avail {
	uint16_t flags;
	uint16_t idx;
	uint16_t ring[];
} __attribute__((packed));

// https://docs.oasis-open.org/virtio/virtio/v1.2/csd01/virtio-v1.2-csd01.html#x1-540008
struct virtq_used_elem {
	uint32_t id;
	uint32_t len;
} __attribute__((packed));

// usedリング (2.7.8 The Virtqueue Used Ring)
// https://docs.oasis-open.org/virtio/virtio/v1.2/csd01/virtio-v1.2-csd01.html#x1-540008
struct virtq_used {
	uint16_t flags;
	uint16_t index;
	struct virtq_used_elem ring[];
} __attribute__((packed));

error_t init_virtio_pci();
