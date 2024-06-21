#include "hardware/virtio/blk.hpp"
#include "graphics/log.hpp"
#include "hardware/virtio/pci.hpp"
#include "hardware/virtio/virtio.hpp"
#include "libs/common/types.hpp"
#include "memory/slab.hpp"
#include "task/task.hpp"

virtio_pci_device* blk_dev = nullptr;

error_t write_to_blk_device(void* buffer, uint64_t sector, uint32_t len)
{
	if (len > SECTOR_SIZE) {
		printk(KERN_ERROR, "Data size is too big.");
		return ERR_INVALID_ARG;
	}

	if (len < SECTOR_SIZE) {
		printk(KERN_ERROR, "Data size is too small.");
		return ERR_INVALID_ARG;
	}

	virtio_blk_req* req =
			(virtio_blk_req*)kmalloc(sizeof(virtio_blk_req), KMALLOC_ZEROED);
	if (req == nullptr) {
		printk(KERN_ERROR, "Failed to allocate memory for virtio_blk_req.");
		return ERR_NO_MEMORY;
	}

	req->type = VIRTIO_BLK_T_OUT;
	req->sector = sector;
	req->reserved = 0;

	memcpy(req->data, buffer, len);

	virtio_entry chain[3];
	chain[0].addr = (uint64_t)req;
	chain[0].len = sizeof(uint32_t) * 2 + sizeof(uint64_t);
	chain[0].write = false;

	chain[1].addr = (uint64_t)&req->data;
	chain[1].len = len;
	chain[1].write = false;

	chain[2].addr = (uint64_t)&req->status;
	chain[2].len = sizeof(uint8_t);
	chain[2].write = true;

	push_virtio_entry(&blk_dev->queues[0], chain, 3);

	notify_virtqueue(*blk_dev, 0);

	switch_next_task(true);

	if (req->status != VIRTIO_BLK_S_OK) {
		printk(KERN_ERROR, "Failed to write to block device.");
		kfree(req);

		return ERR_FAILED_WRITE_TO_DEVICE;
	}

	kfree(req);

	return OK;
}

error_t read_from_blk_device(uint64_t sector, uint32_t len)
{
	if (len > SECTOR_SIZE) {
		printk(KERN_ERROR, "Data size is too big.");
		return ERR_INVALID_ARG;
	}

	if (len < SECTOR_SIZE) {
		printk(KERN_ERROR, "Data size is too small.");
		return ERR_INVALID_ARG;
	}

	virtio_blk_req* req =
			(virtio_blk_req*)kmalloc(sizeof(virtio_blk_req), KMALLOC_ZEROED);
	if (req == nullptr) {
		return ERR_NO_MEMORY;
	}

	req->type = VIRTIO_BLK_T_IN;
	req->sector = sector;

	virtio_entry chain[3];
	chain[0].addr = (uint64_t)req;
	chain[0].len = sizeof(uint32_t) * 2 + sizeof(uint64_t);
	chain[0].write = false;

	chain[1].addr = (uint64_t)&req->data;
	chain[1].len = len;
	chain[1].write = true;

	chain[2].addr = (uint64_t)&req->status;
	chain[2].len = sizeof(uint8_t);
	chain[2].write = true;

	push_virtio_entry(&blk_dev->queues[0], chain, 3);

	notify_virtqueue(*blk_dev, 0);

	// ここでタスクをスリープさせる

	kfree(req);

	return OK;
}

error_t init_blk_device()
{
	void* buffer = kmalloc(sizeof(virtio_pci_device), KMALLOC_ZEROED);
	if (buffer == nullptr) {
		return ERR_NO_MEMORY;
	}

	blk_dev = new (buffer) virtio_pci_device();

	if (IS_ERR(init_virtio_pci_device(blk_dev, VIRTIO_BLK))) {
		return ERR_FAILED_INIT_DEVICE;
	}

	return OK;
}
