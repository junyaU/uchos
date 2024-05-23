#include "hardware/virtio/blk.hpp"
#include "graphics/log.hpp"
#include "hardware/virtio/pci.hpp"
#include "hardware/virtio/virtio.hpp"
#include "libs/common/types.hpp"
#include "memory/slab.hpp"

error_t write_to_blk_device(void* buffer,
							uint64_t sector,
							uint32_t len,
							virtio_pci_device* dev)
{
	virtio_blk_req* req =
			(virtio_blk_req*)kmalloc(sizeof(virtio_blk_req), KMALLOC_ZEROED);
	if (req == nullptr) {
		printk(KERN_ERROR, "Failed to allocate memory for virtio_blk_req.");
		return ERR_NO_MEMORY;
	}

	req->type = VIRTIO_BLK_T_OUT;
	req->reserved = 0;
	req->sector = sector;

	memcpy(req->data, buffer, len);

	virtio_entry chain[3];
	// type, reserved, sector
	chain[0].addr = (uint64_t)req;
	chain[0].len = sizeof(uint32_t) * 2 + sizeof(uint64_t);
	chain[0].write = false;

	// data
	chain[1].addr = (uint64_t)&req->data;
	chain[1].len = len;
	chain[1].write = false;

	// status
	chain[2].addr = (uint64_t)&req->status;
	chain[2].len = sizeof(uint8_t);
	chain[2].write = true;

	push_virtio_entry(&dev->queues[0], chain, 3);

	notify_virtqueue(*dev, 0);

	return OK;
}

error_t read_from_blk_device(void* buffer, uint64_t sector, uint32_t len)
{
	return OK;
}
