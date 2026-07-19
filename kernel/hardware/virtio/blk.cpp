#include "hardware/virtio/blk.hpp"
#include <cstdint>
#include <libs/common/message.hpp>
#include <libs/common/process_id.hpp>
#include <libs/common/types.hpp>
#include "hardware/virtio/pci.hpp"
#include "hardware/virtio/virtio.hpp"
#include "log/log.hpp"
#include "memory/slab.hpp"
#include "task/ipc.hpp"
#include "task/task.hpp"

namespace
{
// Pre-fill a reply so error responses always carry the request id and the
// error kind: an all-zero reply used to collide with cached request id 0 and
// made the FS side memcpy from a null buffer (issue #313).
Message make_blk_reply(const Message& m)
{
	Message reply = { .type = m.data.blk_io.dst_type,
					  .sender = process_ids::VIRTIO_BLK };
	reply.data.blk_io.buf = nullptr;
	reply.data.blk_io.sector = m.data.blk_io.sector;
	reply.data.blk_io.len = m.data.blk_io.len;
	reply.data.blk_io.sequence = m.data.blk_io.sequence;
	reply.data.blk_io.request_id = m.data.blk_io.request_id;
	reply.data.blk_io.result = OK;

	return reply;
}

void handle_read_request(const Message& m)
{
	Message reply = make_blk_reply(m);

	const int sector = m.data.blk_io.sector;
	const int len = m.data.blk_io.len;

	void* buf_ptr = kernel::memory::alloc(len, kernel::memory::ALLOC_ZEROED);
	if (buf_ptr == nullptr) {
		LOG_ERROR("failed to allocate read buffer: len=%d", len);
		reply.data.blk_io.result = ERR_NO_MEMORY;
		kernel::task::send_message(m.sender, reply);
		return;
	}

	char* buf = static_cast<char*>(buf_ptr);

	const error_t err = kernel::hw::virtio::read_from_blk_device(buf, sector, len);
	if (IS_ERR(err)) {
		LOG_ERROR("failed to read from blk device: %d", err);
		kernel::memory::free(buf);
		reply.data.blk_io.result = err;
		kernel::task::send_message(m.sender, reply);
		return;
	}

	reply.data.blk_io.buf = buf;

	kernel::task::send_message(m.sender, reply);
}

void handle_write_request(const Message& m)
{
	Message reply = make_blk_reply(m);

	const int sector = m.data.blk_io.sector;
	const int len = m.data.blk_io.len < SECTOR_SIZE
							? SECTOR_SIZE
							: m.data.blk_io.len;

	const error_t err = kernel::hw::virtio::write_to_blk_device(
			static_cast<const char*>(m.data.blk_io.buf), sector, len);
	if (IS_ERR(err)) {
		LOG_ERROR("failed to write to blk device: %d", err);
		reply.data.blk_io.result = err;
	}

	kernel::memory::free(m.data.blk_io.buf);

	// Always report completion so the FS side can acknowledge the requester
	// instead of claiming success before the device write finished.
	kernel::task::send_message(m.sender, reply);
}
} // namespace

namespace kernel::hw::virtio
{

VirtioPciDevice* blk_dev = nullptr;

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

	void* req_ptr;
	ALLOC_OR_RETURN_ERROR(req_ptr, sizeof(VirtioBlkReq),
						  kernel::memory::ALLOC_ZEROED);
	VirtioBlkReq* req = (VirtioBlkReq*)req_ptr;

	req->type = VIRTIO_BLK_T_OUT;
	req->sector = sector;

	VirtioEntry chain[3];
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

	void* req_ptr;
	ALLOC_OR_RETURN_ERROR(req_ptr, sizeof(VirtioBlkReq),
						  kernel::memory::ALLOC_ZEROED);
	VirtioBlkReq* req = (VirtioBlkReq*)req_ptr;

	req->type = VIRTIO_BLK_T_IN;
	req->sector = sector;

	VirtioEntry chain[3];
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

		return ERR_FAILED_READ_FROM_DEVICE;
	}

	kernel::memory::free(req);

	return OK;
}

error_t init_blk_device()
{
	void* buffer = kernel::memory::alloc(sizeof(VirtioPciDevice),
										 kernel::memory::ALLOC_ZEROED);
	if (buffer == nullptr) {
		return ERR_NO_MEMORY;
	}

	blk_dev = new (buffer) VirtioPciDevice();

	ASSERT_OK(init_virtio_pci_device(blk_dev, VIRTIO_BLK));

	kernel::task::CURRENT_TASK->is_initialized = true;

	return OK;
}

void virtio_blk_service()
{
	kernel::task::Task* t = kernel::task::CURRENT_TASK;

	init_blk_device();

	t->add_msg_handler(MsgType::IPC_READ_FROM_BLK_DEVICE, handle_read_request);
	t->add_msg_handler(MsgType::IPC_WRITE_TO_BLK_DEVICE, handle_write_request);

	t->is_initialized = true;

	kernel::task::process_messages(t);
}

} // namespace kernel::hw::virtio
