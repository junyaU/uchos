/**
 * @file hardware/virtio/pci.hpp
 *
 * @brief VirtIO PCI Device Driver
 *
 * This file defines the structures, functions, and constants used for
 * interacting with VirtIO PCI devices. It provides functionalities for
 * managing VirtIO PCI capabilities, common configurations, and notifications.
 */
#pragma once

#include "hardware/pci.hpp"
#include "hardware/virtio/virtio.hpp"
#include <cstddef>
#include <cstdint>
#include <libs/common/types.hpp>

namespace kernel::hw::pci
{
struct device;
}

namespace kernel::hw::virtio {

/**
 * @brief VirtIO PCI capability types
 * 
 * These constants identify different types of capabilities that
 * can be present in a VirtIO PCI device.
 * @see https://docs.oasis-open.org/virtio/virtio/v1.3/csd01/virtio-v1.3-csd01.html#x1-1390004
 * @{
 */
constexpr int VIRTIO_PCI_CAP_COMMON_CFG = 1;         ///< Common configuration
constexpr int VIRTIO_PCI_CAP_NOTIFY_CFG = 2;         ///< Notifications
constexpr int VIRTIO_PCI_CAP_ISR_CFG = 3;            ///< ISR access
constexpr int VIRTIO_PCI_CAP_DEVICE_CFG = 4;         ///< Device specific configuration
constexpr int VIRTIO_PCI_CAP_PCI_CFG = 5;            ///< PCI configuration access
constexpr int VIRTIO_PCI_CAP_SHARED_MEMORY_CFG = 8;  ///< Shared memory region
constexpr int VIRTIO_PCI_CAP_VENDOR_CFG = 9;         ///< Vendor-specific data
/** @} */

/**
 * @brief Invalid MSI-X vector value
 * 
 * Used to indicate that no MSI-X vector is assigned.
 */
constexpr int NO_VECTOR = 0xffff;

/**
 * @struct virtio_pci_cap
 * @brief VirtIO PCI Capability Structure
 *
 * This structure represents a VirtIO PCI capability.
 *
 * @see
 * https://docs.oasis-open.org/virtio/virtio/v1.3/csd01/virtio-v1.3-csd01.html#x1-1390004
 */
struct virtio_pci_cap {
	union {
		uint32_t data;

		struct {
			uint8_t cap_vndr; /* Generic PCI field: PCI_CAP_ID_VNDR */
			uint8_t cap_next; /* Generic PCI field: next ptr. */
			uint8_t cap_len;  /* Generic PCI field: capability length */
			uint8_t cfg_type; /* Identifies the structure. */
		} __attribute__((packed)) fields;
	} first_dword;

	union {
		uint32_t data;

		struct {
			uint8_t bar;		/* Where to find it. */
			uint8_t id;			/* Multiple capabilities of the same type */
			uint8_t padding[2]; /* Pad to full dword. */
		} __attribute__((packed)) fields;
	} second_dword;

	uint32_t offset; /* Offset within bar. */
	uint32_t length; /* Length of the structure, in bytes. */

	virtio_pci_cap* next;
} __attribute__((packed));

/**
 * @struct virtio_pci_common_cfg
 * @brief VirtIO PCI Common Configuration Structure
 *
 * This structure represents the common configuration for a VirtIO PCI device.
 *
 * @see
 * https://docs.oasis-open.org/virtio/virtio/v1.3/csd01/virtio-v1.3-csd01.html#x1-1390004
 */
struct virtio_pci_common_cfg {
	/* About the whole device. */
	uint32_t device_feature_select; /* read-write */
	uint32_t device_feature;		/* read-only */
	uint32_t driver_feature_select; /* write */
	uint32_t driver_feature;		/* write */
	uint16_t config_msix_vector;	/* read-write */
	uint16_t num_queues;			/* read-only */
	uint8_t device_status;			/* read-write */
	uint8_t config_generation;		/* read-only */

	/* About a specific virtqueue. */
	uint16_t queue_select;			  /* write */
	uint16_t queue_size;			  /* write */
	uint16_t queue_msix_vector;		  /* write */
	uint16_t queue_enable;			  /* write */
	uint16_t queue_notify_off;		  /* read-only */
	uint64_t queue_desc;			  /* read-write */
	uint64_t queue_driver;			  /* read-write */
	uint64_t queue_device;			  /* read-write */
	uint16_t queue_notif_config_data; /* read-only for driver */
	uint16_t queue_reset;			  /* read-write */
} __attribute__((packed));

/**
 * @struct virtio_pci_notify_cap
 * @brief VirtIO PCI Notify Capability Structure
 *
 * This structure represents the notify capability for a VirtIO PCI device.
 *
 * @see
 * https://docs.oasis-open.org/virtio/virtio/v1.3/csd01/virtio-v1.3-csd01.html#x1-1390004
 */
struct virtio_pci_notify_cap {
	struct virtio_pci_cap cap;
	uint32_t notify_off_multiplier; /* Multiplier for queue_notify_off. */
} __attribute__((packed));

/**
 * @struct virtio_pci_cfg_cap
 * @brief VirtIO PCI Configuration Capability Structure
 *
 * This structure represents the PCI configuration capability for a VirtIO PCI
 * device.
 *
 * @see
 * https://docs.oasis-open.org/virtio/virtio/v1.3/csd01/virtio-v1.3-csd01.html#x1-1390004
 */
struct virtio_pci_cfg_cap {
	struct virtio_pci_cap cap;
	uint8_t pci_cfg_data[4]; /* Offset within bar. */
} __attribute__((packed));

/**
 * @struct virtio_pci_device
 * @brief VirtIO PCI Device Structure
 *
 * This structure represents a VirtIO PCI device, including its capabilities,
 * configurations, and associated Virtqueues.
 */
struct virtio_pci_device {
	kernel::hw::pci::device* dev;				   /* PCI device. */
	virtio_pci_cap* caps;			   /* Capabilities. */
	virtio_pci_common_cfg* common_cfg; /* Common configuration. */
	virtio_pci_notify_cap* notify_cfg; /* Notifications. */
	virtio_virtqueue* queues;		   /* Virtqueues. */
	size_t num_queues;				   /* Number of virtqueues. */
	uintptr_t notify_base;			   /* Base address for notifications. */
};

/**
 * @brief Get a pointer to a VirtIO PCI capability
 * 
 * Maps a VirtIO capability structure from the PCI BAR space.
 * 
 * @tparam T Type of the capability structure
 * @param virtio_dev VirtIO PCI device
 * @return T* Pointer to the mapped capability structure
 * 
 * @note The BAR must be memory-mapped for this to work correctly
 */
template<typename T>
T* get_virtio_pci_capability(virtio_pci_device& virtio_dev)
{
	uint64_t bar_addr = kernel::hw::pci::read_base_address_register(
			*virtio_dev.dev, virtio_dev.caps->second_dword.fields.bar);
	bar_addr &= ~0xfff;

	return reinterpret_cast<T*>(bar_addr + virtio_dev.caps->offset);
}

/**
 * @brief Find and enumerate VirtIO PCI capabilities
 * 
 * Scans the PCI configuration space to find all VirtIO capabilities
 * and builds a linked list of them.
 * 
 * @param virtio_dev VirtIO PCI device to scan
 * @return size_t Number of capabilities found
 */
size_t find_virtio_pci_cap(virtio_pci_device& virtio_dev);

/**
 * @brief Set up VirtIO PCI device capabilities
 * 
 * Configures the device based on the discovered capabilities,
 * including common configuration, notification, and ISR structures.
 * 
 * @param virtio_dev VirtIO PCI device to configure
 * @return error_t Error code (kSuccess on success)
 */
error_t set_virtio_pci_capability(virtio_pci_device& virtio_dev);

/**
 * @brief Notify the device about available buffers in a virtqueue
 * 
 * Writes to the appropriate notification register to inform the
 * device that new buffers are available in the specified queue.
 * 
 * @param virtio_dev VirtIO PCI device
 * @param queue_idx Index of the virtqueue to notify
 */
void notify_virtqueue(virtio_pci_device& virtio_dev, size_t queue_idx);

} // namespace kernel::hw::virtio
