#pragma once

#include "hardware/pci.hpp"
#include <cstddef>
#include <cstdint>
#include <libs/common/types.hpp>

namespace pci
{
struct device;
}

/* Common configuration */
constexpr int VIRTIO_PCI_CAP_COMMON_CFG = 1;
/* Notifications */
constexpr int VIRTIO_PCI_CAP_NOTIFY_CFG = 2;
/* ISR access */
constexpr int VIRTIO_PCI_CAP_ISR_CFG = 3;
/* Device specific configuration */
constexpr int VIRTIO_PCI_CAP_DEVICE_CFG = 4;
/* PCI configuration access */
constexpr int VIRTIO_PCI_CAP_PCI_CFG = 5;
/* Shared memory region */
constexpr int VIRTIO_PCI_CAP_SHARED_MEMORY_CFG = 8;
/* Vendor-specific data */
constexpr int VIRTIO_PCI_CAP_VENDOR_CFG = 9;

// https://docs.oasis-open.org/virtio/virtio/v1.2/csd01/virtio-v1.2-csd01.html#x1-1240004
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

// https://docs.oasis-open.org/virtio/virtio/v1.2/csd01/virtio-v1.2-csd01.html#x1-1240004
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

// https://docs.oasis-open.org/virtio/virtio/v1.2/csd01/virtio-v1.2-csd01.html#x1-1240004
struct virtio_pci_notify_cap {
	struct virtio_pci_cap cap;
	uint32_t notify_off_multiplier; /* Multiplier for queue_notify_off. */
} __attribute__((packed));

// https://docs.oasis-open.org/virtio/virtio/v1.2/csd01/virtio-v1.2-csd01.html#x1-1240004
struct virtio_pci_cfg_cap {
	struct virtio_pci_cap cap;
	uint8_t pci_cfg_data[4]; /* Offset within bar. */
} __attribute__((packed));

template<typename T>
T* get_virtio_pci_capability(pci::device& virtio_dev, virtio_pci_cap* caps)
{
	uint64_t bar_addr = pci::read_base_address_register(
			virtio_dev, caps->second_dword.fields.bar);
	bar_addr = bar_addr & 0xffff'ffff'ffff'f000U;

	return reinterpret_cast<T*>(bar_addr + caps->offset);
}

size_t find_virtio_pci_cap(pci::device& virtio_dev, virtio_pci_cap** caps);

error_t set_virtio_pci_capability(pci::device& virtio_dev, virtio_pci_cap* cap);