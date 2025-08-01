#include "hardware/virtio/virtio.hpp"
#include "graphics/log.hpp"
#include "hardware/pci.hpp"
#include "hardware/virtio/blk.hpp"
#include "hardware/virtio/pci.hpp"
#include "interrupt/vector.hpp"
#include <cstddef>
#include <cstdint>
#include <libs/common/types.hpp>

namespace kernel::hw::virtio {

error_t init_VirtioPciDevice(VirtioPciDevice* virtio_dev, int device_type)
{
	kernel::hw::pci::Device* dev = nullptr;
	for (int i = 0; i < kernel::hw::pci::num_devices; ++i) {
		if (kernel::hw::pci::devices[i].is_virtio()) {
			dev = &kernel::hw::pci::devices[i];
			break;
		}
	}

	if (dev == nullptr) {
		LOG_ERROR("No virtio device found");
		return ERR_NO_DEVICE;
	}

	const uint8_t bsp_lapic_id = *reinterpret_cast<uint32_t*>(0xfee00020) >> 24;
	kernel::hw::pci::configure_msi_fixed_destination(
			*dev, bsp_lapic_id, kernel::hw::pci::MsiTriggerMode::EDGE,
			kernel::hw::pci::MsiDeliveryMode::FIXED, kernel::interrupt::InterruptVector::VIRTIO, 0);
	kernel::hw::pci::configure_msi_fixed_destination(
			*dev, bsp_lapic_id, kernel::hw::pci::MsiTriggerMode::EDGE,
			kernel::hw::pci::MsiDeliveryMode::FIXED, kernel::interrupt::InterruptVector::VIRTQUEUE, 0);

	virtio_dev->dev = dev;

	find_VirtioPciCap(*virtio_dev);

	return set_VirtioPciCapability(*virtio_dev);
}

int push_VirtioEntry(VirtioVirtqueue* queue,
					  VirtioEntry* entry_chain,
					  size_t num_entries)
{
	if (queue->num_free_desc < num_entries) {
		while (queue->last_device_idx != queue->device->index) {
			VirtqDeviceElem* elem =
					&queue->device->ring[queue->last_device_idx % queue->num_desc];

			int num_freed = 0;
			int next_idx = elem->id;
			while (true) {
				VirtqDesc* desc = &queue->desc[next_idx];
				++num_freed;

				if ((desc->flags & VIRTQ_DESC_F_NEXT) == 0) {
					break;
				}

				next_idx = desc->next;
			}

			queue->top_free_idx = elem->id;
			queue->num_free_desc += num_freed;
			++queue->last_device_idx;
		}
	}

	if (queue->num_free_desc < num_entries) {
		return ERR_NO_MEMORY;
	}

	const int top_free_idx = queue->top_free_idx;
	int desc_idx = top_free_idx;
	VirtqDesc* desc = nullptr;

	for (int i = 0; i < num_entries; ++i) {
		VirtioEntry* entry = &entry_chain[i];
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

	__asm__ volatile("sfence" ::: "memory");

	++queue->driver->index;

	return top_free_idx;
}

int pop_VirtioEntry(VirtioVirtqueue* queue,
					 VirtioEntry* entry_chain,
					 size_t num_entries)
{
	VirtqDeviceElem* elem =
			&queue->device->ring[queue->last_device_idx % queue->num_desc];

	int desc_idx = elem->id;
	VirtqDesc* desc = nullptr;
	int num_pop = 0;

	while (num_pop < num_entries) {
		desc = &queue->desc[desc_idx];
		entry_chain[num_pop].index = desc_idx;
		entry_chain[num_pop].addr = desc->addr;
		entry_chain[num_pop].len = desc->len;
		entry_chain[num_pop].write = (desc->flags & VIRTQ_DESC_F_WRITE) != 0;

		++num_pop;

		if ((desc->flags & VIRTQ_DESC_F_NEXT) == 0) {
			break;
		}

		desc_idx = desc->next;
	}

	if (desc == nullptr) {
		return 0;
	}

	desc->next = queue->top_free_idx;
	queue->top_free_idx = elem->id;
	queue->num_free_desc += num_pop;

	++queue->last_device_idx;

	return num_pop;
}

error_t init_virtqueue(VirtioVirtqueue* queue,
					   size_t index,
					   size_t num_desc,
					   uintptr_t desc_addr,
					   uintptr_t driver_ring_addr,
					   uintptr_t device_ring_addr)
{
	queue->index = index;
	queue->num_desc = num_desc;
	queue->num_free_desc = num_desc;
	queue->desc = reinterpret_cast<VirtqDesc*>(desc_addr);
	queue->driver = reinterpret_cast<VirtqDriver*>(driver_ring_addr);
	queue->device = reinterpret_cast<VirtqDevice*>(device_ring_addr);

	for (int i = 0; i < num_desc; ++i) {
		queue->desc[i].addr = 0;
		queue->desc[i].len = 0;
		queue->desc[i].flags = 0;
		queue->desc[i].next = (i + 1) % num_desc;
	}

	return OK;
}

} // namespace kernel::hw::virtio
