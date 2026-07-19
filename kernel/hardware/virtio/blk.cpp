#include "hardware/virtio/blk.hpp"
#include <cstdint>
#include <libs/common/message.hpp>
#include <libs/common/process_id.hpp>
#include <libs/common/types.hpp>
#include <utility>
#include "error.hpp"
#include "hardware/virtio/pci.hpp"
#include "hardware/virtio/virtio.hpp"
#include "interrupt/routing.hpp"
#include "interrupt/vector.hpp"
#include "log/log.hpp"
#include "memory/slab.hpp"
#include "task/ipc.hpp"
#include "task/task.hpp"

namespace
{
// Pre-fill a reply that echoes the request geometry; the request/reply
// pairing itself is Message::correlation now (issue #314 Stage B), so the
// old dst_type/request_id/sequence bookkeeping is gone.
Message make_blk_reply(const Message& m)
{
	Message reply = { .type = m.type, .sender = process_ids::VIRTIO_BLK };
	reply.data.blk.sector = m.data.blk.sector;
	reply.data.blk.len = m.data.blk.len;
	reply.result = OK;

	return reply;
}

void handle_read_request(const Message& m)
{
	Message reply = make_blk_reply(m);

	const int sector = m.data.blk.sector;
	const int len = m.data.blk.len;

	auto buf = kernel::memory::make_kbuf(len, kernel::memory::ALLOC_ZEROED);
	if (!buf) {
		LOG_ERROR("failed to allocate read buffer: len=%d", len);
		reply.result = ERR_NO_MEMORY;
		kernel::task::reply(m, &reply);
		return;
	}

	const error_t err = kernel::hw::virtio::read_from_blk_device(
			static_cast<char*>(buf.get()), sector, len);
	if (IS_ERR(err)) {
		LOG_ERROR("failed to read from blk device: %d", err);
		reply.result = err;
		kernel::task::reply(m, &reply);
		return;
	}

	// The data moves to the caller as the reply's OOL payload
	kernel::task::reply_with_ool(m, &reply, std::move(buf), len);
}

void handle_write_request(const Message& m)
{
	Message reply = make_blk_reply(m);

	// The request's OOL payload is ours to free once the write is done
	kernel::memory::unique_kbuf<> buf{ reinterpret_cast<void*>(m.ool.addr) };
	if (!buf || m.ool.size == 0) {
		reply.result = ERR_INVALID_ARG;
		kernel::task::reply(m, &reply);
		return;
	}

	const int sector = m.data.blk.sector;
	const int len = m.data.blk.len < SECTOR_SIZE ? SECTOR_SIZE : m.data.blk.len;

	const error_t err = kernel::hw::virtio::write_to_blk_device(
			static_cast<const char*>(buf.get()), sector, len);
	if (IS_ERR(err)) {
		LOG_ERROR("failed to write to blk device: %d", err);
		reply.result = err;
	}

	// Always report completion so the FS side can acknowledge the requester
	// instead of claiming success before the device write finished.
	kernel::task::reply(m, &reply);
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
	RETURN_IF_ERROR(validate_length(len));

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

	// Sleep until the completion doorbell. The old bare sleep was woken by
	// ANY incoming message, and a request arriving mid-wait made the
	// zero-initialized status (VIRTIO_BLK_S_OK == 0) read as premature
	// success before the device finished the DMA (issue #314 Stage A).
	kernel::task::wait_notification(
			kernel::task::notify_bit(kernel::task::NotifyType::VIRTIO_BLK));

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
	RETURN_IF_ERROR(validate_length(len));

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

	// See write_to_blk_device: wait for the completion doorbell instead of
	// a bare sleep that any sender could cut short
	kernel::task::wait_notification(
			kernel::task::notify_bit(kernel::task::NotifyType::VIRTIO_BLK));

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

	RETURN_IF_ERROR(init_virtio_pci_device(blk_dev, VIRTIO_BLK));

	// Wire the completion interrupt to this service's doorbell; the
	// interrupt layer itself no longer knows any destination PID
	RETURN_IF_ERROR(kernel::interrupt::register_irq_notification(
			kernel::interrupt::InterruptVector::VIRTQUEUE_BLK,
			kernel::task::CURRENT_TASK->id, kernel::task::NotifyType::VIRTIO_BLK));

	kernel::task::CURRENT_TASK->is_initialized = true;

	return OK;
}

void virtio_blk_service()
{
	kernel::task::Task* t = kernel::task::CURRENT_TASK;

	init_blk_device();

	t->add_msg_handler(MsgType::BLK_READ, handle_read_request);
	t->add_msg_handler(MsgType::BLK_WRITE, handle_write_request);

	t->is_initialized = true;

	kernel::task::process_messages(t);
}

} // namespace kernel::hw::virtio
