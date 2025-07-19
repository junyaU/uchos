#include "hardware/virtio/blk.hpp"
#include "graphics/log.hpp"
#include "hardware/virtio/pci.hpp"
#include "hardware/virtio/virtio.hpp"
#include "memory/slab.hpp"
#include "task/ipc.hpp"
#include "task/task.hpp"
#include <cstdint>
#include <cstring>
#include <libs/common/message.hpp>
#include <libs/common/process_id.hpp>
#include <libs/common/types.hpp>

namespace
{
void handle_read_request(const message& m)
{
	message reply = { .type = m.data.blk_io.dst_type, .sender = process_ids::VIRTIO_BLK };

	const int sector = m.data.blk_io.sector;
	const int len = m.data.blk_io.len;

	char* buf = static_cast<char*>(kernel::memory::alloc(len, kernel::memory::ALLOC_ZEROED));
	if (buf == nullptr) {
		LOG_ERROR("failed to allocate buffer");
		return;
	}

	if (IS_ERR(kernel::hw::virtio::read_from_blk_device(buf, sector, len))) {
		LOG_ERROR("failed to read from blk device");
		kernel::memory::free(buf);
		kernel::task::send_message(m.sender, reply);
		return;
	}

	reply.data.blk_io.buf = buf;
	reply.data.blk_io.sector = sector;
	reply.data.blk_io.len = len;
	reply.data.blk_io.sequence = m.data.blk_io.sequence;
	reply.data.blk_io.request_id = m.data.blk_io.request_id;

	kernel::task::send_message(m.sender, reply);
}

void handle_write_request(const message& m)
{
	const int sector = m.data.blk_io.sector;
	const int len = m.data.blk_io.len < kernel::hw::virtio::SECTOR_SIZE
	                    ? kernel::hw::virtio::SECTOR_SIZE
	                    : m.data.blk_io.len;

	if (IS_ERR(kernel::hw::virtio::write_to_blk_device(
	        static_cast<const char*>(m.data.blk_io.buf), sector, len))) {
		LOG_ERROR("failed to write to blk device");
	}

	kernel::memory::free(m.data.blk_io.buf);
}
}  // namespace

namespace kernel::hw::virtio
{

virtio_pci_device* blk_dev = nullptr;

error_t validate_length(uint32_t len)
{
	if (len < SECTOR_SIZE) {
		LOG_ERROR("Data size is too small: %d bytes. Minimum is %d bytes.", len, SECTOR_SIZE);
		return ERR_INVALID_ARG;
	}

	return OK;
}

error_t write_to_blk_device(const char* buffer, uint64_t sector, uint32_t len)
{
	ASSERT_OK(validate_length(len));

	virtio_blk_req* req = (virtio_blk_req*)kernel::memory::alloc(sizeof(virtio_blk_req),
	                                                             kernel::memory::ALLOC_ZEROED);
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

	kernel::task::switch_next_task(true);

	if (req->status != VIRTIO_BLK_S_OK) {
		LOG_ERROR("Failed to write to block device.");
		kernel::memory::free(req);

		return ERR_FAILED_WRITE_TO_DEVICE;
	}

	kernel::memory::free(req);

	return OK;
}

error_t read_from_blk_device(const char* buffer, uint64_t sector, uint32_t len)
{
	ASSERT_OK(validate_length(len));

	virtio_blk_req* req = (virtio_blk_req*)kernel::memory::alloc(sizeof(virtio_blk_req),
	                                                             kernel::memory::ALLOC_ZEROED);
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

	kernel::task::switch_next_task(true);

	if (req->status != VIRTIO_BLK_S_OK) {
		LOG_ERROR("Failed to read from block device. status: %d", req->status);
		kernel::memory::free(req);

		return ERR_FAILED_WRITE_TO_DEVICE;
	}

	kernel::memory::free(req);

	return OK;
}

error_t init_blk_device()
{
	void* buffer = kernel::memory::alloc(sizeof(virtio_pci_device), kernel::memory::ALLOC_ZEROED);
	if (buffer == nullptr) {
		return ERR_NO_MEMORY;
	}

	blk_dev = new (buffer) virtio_pci_device();

	ASSERT_OK(init_virtio_pci_device(blk_dev, VIRTIO_BLK));

	kernel::task::CURRENT_TASK->is_initilized = true;

	return OK;
}

void virtio_blk_service()
{
	kernel::task::task* t = kernel::task::CURRENT_TASK;

	init_blk_device();

	t->add_msg_handler(msg_t::IPC_READ_FROM_BLK_DEVICE, handle_read_request);
	t->add_msg_handler(msg_t::IPC_WRITE_TO_BLK_DEVICE, handle_write_request);

	t->is_initilized = true;

	kernel::task::process_messages(t);
}

}  // namespace kernel::hw::virtio
