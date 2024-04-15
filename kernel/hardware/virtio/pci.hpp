#pragma once

#include <cstdint>

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
	uint8_t cap_vndr;	/* Generic PCI field: PCI_CAP_ID_VNDR */
	uint8_t cap_next;	/* Generic PCI field: next ptr. */
	uint8_t cap_len;	/* Generic PCI field: capability length */
	uint8_t cfg_type;	/* Identifies the structure. */
	uint8_t bar;		/* Where to find it. */
	uint8_t id;			/* Multiple capabilities of the same type */
	uint8_t padding[2]; /* Pad to full dword. */
	uint32_t offset;	/* Offset within bar. */
	uint32_t length;	/* Length of the structure, in bytes. */
};
