/**
 * @file hardware/virtio/virtio.hpp
 *
 * @brief VirtIO Device Driver
 *
 * This file defines the structures, functions, and constants used for
 * interacting with VirtIO devices. It provides functionalities for initializing
 * VirtIO PCI devices, managing Virtqueues, and handling descriptors.
 */

#pragma once

#include <cstddef>
#include <cstdint>
#include <libs/common/types.hpp>

namespace kernel::hw::virtio {

// Forward declaration
struct virtio_pci_device;

// Device types
constexpr int VIRTIO_BLK = 1;

// https://docs.oasis-open.org/virtio/virtio/v1.3/csd01/virtio-v1.3-csd01.html#x1-110001
constexpr int VIRTIO_STATUS_ACKNOWLEDGE = 1;
constexpr int VIRTIO_STATUS_DRIVER = 2;
constexpr int VIRTIO_STATUS_DRIVER_OK = 4;
constexpr int VIRTIO_STATUS_FEATURES_OK = 8;
constexpr int VIRTIO_STATUS_DEVICE_NEEDS_RESET = 64;
constexpr int VIRTIO_STATUS_FAILED = 128;

// https://docs.oasis-open.org/virtio/virtio/v1.3/csd01/virtio-v1.3-csd01.html#x1-7080006
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

/**
 * @struct virtq_desc
 * @brief Virtqueue Descriptor
 *
 * This structure describes a single descriptor in a Virtqueue.
 * @see
 * https://docs.oasis-open.org/virtio/virtio/v1.3/csd01/virtio-v1.3-csd01.html#x1-490006
 */
struct virtq_desc {
	uint64_t addr;	/* Address of buffer */
	uint32_t len;	/* Length of buffer */
	uint16_t flags; /* The flags as indicated above */
	uint16_t next;	/* Next field if flags & NEXT */
} __attribute__((packed));

/**
 * @struct virtq_driver
 * @brief Virtqueue Driver Area
 *
 * This structure describes the driver area of a Virtqueue.
 *
 * @see
 * https://docs.oasis-open.org/virtio/virtio/v1.3/csd01/virtio-v1.3-csd01.html#x1-490006
 */
struct virtq_driver {
	uint16_t flags;
	uint16_t index;
	uint16_t ring[];
} __attribute__((packed));

/**
 * @struct virtq_device_elem
 * @brief Virtqueue Device Element
 *
 * This structure describes an element in the device area of a Virtqueue.
 *
 * @see
 * https://docs.oasis-open.org/virtio/virtio/v1.3/csd01/virtio-v1.3-csd01.html#x1-540008
 */
struct virtq_device_elem {
	uint32_t id;
	uint32_t len;
} __attribute__((packed));

/**
 * @struct virtq_device
 * @brief Virtqueue Device Area
 *
 * This structure describes the device area of a Virtqueue.
 *
 * @see
 * https://docs.oasis-open.org/virtio/virtio/v1.3/csd01/virtio-v1.3-csd01.html#x1-540008
 */
struct virtq_device {
	uint16_t flags;
	uint16_t index;
	struct virtq_device_elem ring[];
} __attribute__((packed));

/**
 * @struct virtio_virtqueue
 * @brief Virtqueue Management Structure
 *
 * This structure manages a Virtqueue, including its descriptors,
 * driver area, and device area.
 */
struct virtio_virtqueue {
	size_t index;
	size_t num_desc;
	size_t num_free_desc;
	size_t top_free_idx;
	size_t last_driver_idx;
	uint16_t last_device_idx;
	virtq_desc* desc;
	virtq_driver* driver;
	virtq_device* device;
};

/**
 * @struct virtio_entry
 * @brief Virtqueue Entry
 *
 * This structure represents an entry in the Virtqueue.
 */
struct virtio_entry {
	uint32_t index;
	uintptr_t addr;
	uint32_t len;
	bool write;
};

constexpr size_t calc_desc_area_size(size_t num_desc) { return num_desc * 16; }

constexpr size_t calc_driver_ring_size(size_t num_desc) { return num_desc * 2 + 6; }

constexpr size_t calc_device_ring_size(size_t num_desc) { return num_desc * 8 + 6; }

error_t init_virtio_pci_device(virtio_pci_device* virtio_dev, int device_type);

error_t init_virtqueue(virtio_virtqueue* queue,
					   size_t index,
					   size_t num_desc,
					   uintptr_t desc_addr,
					   uintptr_t driver_ring_addr,
					   uintptr_t device_ring_addr);

int push_virtio_entry(virtio_virtqueue* queue,
					  virtio_entry* entry_chain,
					  size_t num_entries);

int pop_virtio_entry(virtio_virtqueue* queue,
					 virtio_entry* entry_chain,
					 size_t num_entries);

} // namespace kernel::hw::virtio
