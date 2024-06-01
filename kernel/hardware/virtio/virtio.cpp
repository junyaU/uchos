#include "hardware/virtio/virtio.hpp"
#include "graphics/log.hpp"
#include "hardware/pci.hpp"
#include "hardware/virtio/blk.hpp"
#include "hardware/virtio/pci.hpp"
#include "interrupt/vector.hpp"
#include <cstddef>
#include <cstdint>
#include <libs/common/types.hpp>

error_t init_virtio_pci()
{
	pci::device* virtio_dev = nullptr;
	for (int i = 0; i < pci::num_devices; i++) {
		if (pci::devices[i].is_vertio()) {
			virtio_dev = &pci::devices[i];
			break;
		}
	}

	if (virtio_dev == nullptr) {
		printk(KERN_ERROR, "No virtio device found");
		return ERR_NO_DEVICE;
	}

	const uint8_t bsp_lapic_id = *reinterpret_cast<uint32_t*>(0xfee00020) >> 24;
	pci::configure_msi_fixed_destination(
			*virtio_dev, bsp_lapic_id, pci::msi_trigger_mode::EDGE,
			pci::msi_delivery_mode::FIXED, interrupt_vector::VIRTIO, 0);
	pci::configure_msi_fixed_destination(
			*virtio_dev, bsp_lapic_id, pci::msi_trigger_mode::EDGE,
			pci::msi_delivery_mode::FIXED, interrupt_vector::VIRTQUEUE, 0);

	virtio_pci_device virtio_pci_dev = { .dev = virtio_dev };

	find_virtio_pci_cap(virtio_pci_dev, &virtio_pci_dev.caps);

	error_t err = set_virtio_pci_capability(virtio_pci_dev);
	if (IS_ERR(err)) {
		printk(KERN_ERROR, "Failed to set virtio pci capability");
		return err;
	}

	write_to_blk_device((void*)"", 1, 6, &virtio_pci_dev);

	return OK;
}

int push_virtio_entry(virtio_virtqueue* queue,
					  virtio_entry* entry_chain,
					  size_t num_entries)
{
	if (queue->num_free_desc < num_entries) {
		// TODO: handle this case
		printk(KERN_ERROR, "Not enough free descriptors");
		return -1;
	}

	int top_free_idx = queue->top_free_idx;
	int desc_idx = top_free_idx;
	virtq_desc* desc = nullptr;

	for (int i = 0; i < num_entries; ++i) {
		virtio_entry* entry = &entry_chain[i];
		entry->index = desc_idx;

		desc = &queue->desc[desc_idx];
		desc->addr = entry->addr;
		desc->len = entry->len;

		if (i + 1 < num_entries) {
			desc->flags = VIRTQ_DESC_F_NEXT;
		} else {
			queue->top_free_idx = desc->next;
			desc->flags = 0;
			desc->next = 0;
		}

		if (entry->write) {
			desc->flags |= VIRTQ_DESC_F_WRITE;
		}

		desc_idx = desc->next;
		queue->num_free_desc--;
	}

	queue->driver->ring[queue->driver->index % queue->num_desc] = top_free_idx;

	// memory barrier
	asm volatile("sfence" ::: "memory");

	queue->driver->index++;

	return top_free_idx;
}

error_t init_virtqueue(virtio_virtqueue* queue,
					   size_t index,
					   size_t num_desc,
					   uintptr_t desc_addr,
					   uintptr_t driver_ring_addr,
					   uintptr_t device_ring_addr)
{
	queue->index = index;
	queue->num_desc = num_desc;
	queue->num_free_desc = num_desc;
	queue->desc = reinterpret_cast<virtq_desc*>(desc_addr);
	queue->driver = reinterpret_cast<virtq_driver*>(driver_ring_addr);
	queue->device = reinterpret_cast<virtq_device*>(device_ring_addr);

	for (int i = 0; i < num_desc; ++i) {
		queue->desc[i].addr = 0;
		queue->desc[i].len = 0;
		queue->desc[i].flags = 0;
		queue->desc[i].next = (i + 1) % num_desc;
	}

	return OK;
}
