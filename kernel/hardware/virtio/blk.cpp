#include "hardware/virtio/blk.hpp"
#include "graphics/log.hpp"
#include "hardware/virtio/pci.hpp"
#include "hardware/virtio/virtio.hpp"
#include "memory/slab.hpp"
#include "task/ipc.hpp"
#include "task/task.hpp"
#include <libs/common/message.hpp>
#include <libs/common/process_id.hpp>
#include <libs/common/types.hpp>

namespace
{
void handle_read_request(const message& m)
{
	message send_m = { .type = m.data.blk_io.dst_type,
					   .sender = process_ids::VIRTIO_BLK };

	const int sector = m.data.blk_io.sector;
	const int len = m.data.blk_io.len;

	char* buf = static_cast<char*>(kmalloc(len, kernel::memory::KMALLOC_ZEROED));
	if (buf == nullptr) {
		LOG_ERROR("failed to allocate buffer");
		return;
	}

	if (IS_ERR(read_from_blk_device(buf, sector, len))) {
		LOG_ERROR("failed to read from blk device");
		kfree(buf);
		send_message(m.sender, send_m);
	}

	send_m.data.blk_io.buf = buf;
	send_m.data.blk_io.sector = sector;
	send_m.data.blk_io.len = len;
	send_m.data.blk_io.sequence = m.data.blk_io.sequence;
	send_m.data.blk_io.request_id = m.data.blk_io.request_id;

	send_message(m.sender, send_m);
}

void handle_write_request(const message& m)
{
	const int sector = m.data.blk_io.sector;
	const int len =
			m.data.blk_io.len < SECTOR_SIZE ? SECTOR_SIZE : m.data.blk_io.len;

	char buf[512];
	memcpy(buf, m.data.blk_io.buf, len);

	if (IS_ERR(write_to_blk_device(buf, sector, len))) {
		LOG_ERROR("failed to write to blk device");
	}
}
} // namespace

virtio_pci_device* blk_dev = nullptr;

error_t validate_length(uint32_t len)
{
	if (len < SECTOR_SIZE) {
		LOG_ERROR("Data size is too small: %d bytes. Minimum is %d bytes.", len,
				  SECTOR_SIZE);
		return ERR_INVALID_ARG;
	}

	return OK;
}

error_t write_to_blk_device(const char* buffer, uint64_t sector, uint32_t len)
{
	ASSERT_OK(validate_length(len));

	virtio_blk_req* req =
			(virtio_blk_req*)kmalloc(sizeof(virtio_blk_req), kernel::memory::KMALLOC_ZEROED);
	if (req == nullptr) {
		return ERR_NO_MEMORY;
	}

	req->type = VIRTIO_BLK_T_OUT;
	req->sector = sector;

	virtio_entry chain[3];
	chain[0].addr = (uint64_t)req;
	chain[0].len = sizeof(uint32_t) * 2 + sizeof(uint64_t);
	chain[0].write = false;

	chain[1].addr = (uint64_t)buffer;
	chain[1].len = len;
	chain[1].write = false;

	chain[2].addr = (uint64_t)&req->status;
	chain[2].len = sizeof(uint8_t);
	chain[2].write = true;

	push_virtio_entry(&blk_dev->queues[0], chain, 3);

	notify_virtqueue(*blk_dev, 0);

	switch_next_task(true);

	if (req->status != VIRTIO_BLK_S_OK) {
		LOG_ERROR("Failed to write to block device.");
		kfree(req);

		return ERR_FAILED_WRITE_TO_DEVICE;
	}

	kfree(req);

	return OK;
}

error_t read_from_blk_device(const char* buffer, uint64_t sector, uint32_t len)
{
	ASSERT_OK(validate_length(len));

	virtio_blk_req* req =
			(virtio_blk_req*)kmalloc(sizeof(virtio_blk_req), kernel::memory::KMALLOC_ZEROED);
	if (req == nullptr) {
		return ERR_NO_MEMORY;
	}

	req->type = VIRTIO_BLK_T_IN;
	req->sector = sector;

	virtio_entry chain[3];
	chain[0].addr = (uint64_t)req;
	chain[0].len = sizeof(uint32_t) * 2 + sizeof(uint64_t);
	chain[0].write = false;

	chain[1].addr = (uint64_t)buffer;
	chain[1].len = len;
	chain[1].write = true;

	chain[2].addr = (uint64_t)&req->status;
	chain[2].len = sizeof(uint8_t);
	chain[2].write = true;

	push_virtio_entry(&blk_dev->queues[0], chain, 3);

	notify_virtqueue(*blk_dev, 0);

	switch_next_task(true);

	if (req->status != VIRTIO_BLK_S_OK) {
		LOG_ERROR("Failed to read from block device. status: %d", req->status);
		kfree(req);

		return ERR_FAILED_WRITE_TO_DEVICE;
	}

	kfree(req);

	return OK;
}

error_t init_blk_device()
{
	void* buffer = kmalloc(sizeof(virtio_pci_device), kernel::memory::KMALLOC_ZEROED);
	if (buffer == nullptr) {
		return ERR_NO_MEMORY;
	}

	blk_dev = new (buffer) virtio_pci_device();

	ASSERT_OK(init_virtio_pci_device(blk_dev, VIRTIO_BLK));

	CURRENT_TASK->is_initilized = true;

	return OK;
}

void virtio_blk_task()
{
	task* t = CURRENT_TASK;

	init_blk_device();

	t->add_msg_handler(msg_t::IPC_READ_FROM_BLK_DEVICE, handle_read_request);
	t->add_msg_handler(msg_t::IPC_WRITE_TO_BLK_DEVICE, handle_write_request);

	t->is_initilized = true;

	process_messages(t);
}
