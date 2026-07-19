#include "hardware/virtio/net.hpp"
#include <cstddef>
#include <libs/common/types.hpp>
#include "error.hpp"
#include "hardware/virtio/pci.hpp"
#include "hardware/virtio/virtio.hpp"
#include "interrupt/routing.hpp"
#include "interrupt/vector.hpp"
#include "libs/common/message.hpp"
#include "log/log.hpp"
#include "memory/slab.hpp"
#include "net/host.hpp"
#include "task/ipc.hpp"
#include "task/task.hpp"

namespace kernel::hw::virtio
{

VirtioPciDevice* net_dev = nullptr;
// NIC hardware address; read from virtio config space and handed to the
// protocol stack via kernel::net::set_host_mac()
namespace
{
uint8_t mac_addr[6] = { 0 };
} // namespace
VirtioVirtqueue* rx_queue = nullptr;
VirtioVirtqueue* tx_queue = nullptr;

error_t setup_rx_buffers()
{
	for (size_t i = 0; i < NUM_RX_BUFFERS; ++i) {
		void* buf;
		ALLOC_OR_RETURN_ERROR(buf, sizeof(VirtioNetReq),
							  kernel::memory::ALLOC_ZEROED);

		VirtioEntry chain[1];
		chain[0].addr = (uint64_t)buf;
		chain[0].len = sizeof(VirtioNetReq);
		chain[0].write = true;

		push_virtio_entry(rx_queue, chain, 1);
	}

	notify_virtqueue(*net_dev, rx_queue->index);

	return OK;
}

error_t read_mac_address()
{
	VirtioPciCap cap = net_dev->device_cfg->cap;
	uint64_t bar_addr = kernel::hw::pci::read_base_address_register(
			*net_dev->dev, cap.second_dword.fields.bar);
	bar_addr = bar_addr & ~0xfff;

	VirtioNetConfig* net_cfg =
			reinterpret_cast<VirtioNetConfig*>(bar_addr + cap.offset);

	memcpy(mac_addr, net_cfg->mac, 6);

	return OK;
}

error_t init_virtio_net_device()
{
	void* buffer = kernel::memory::alloc(sizeof(VirtioPciDevice),
										 kernel::memory::ALLOC_ZEROED);
	if (buffer == nullptr) {
		return ERR_NO_MEMORY;
	}

	net_dev = new (buffer) VirtioPciDevice();
	RETURN_IF_ERROR(init_virtio_pci_device(net_dev, VIRTIO_NET));

	rx_queue = &net_dev->queues[0];
	tx_queue = &net_dev->queues[1];

	kernel::task::CURRENT_TASK->is_initialized = true;

	return OK;
}

error_t transmit_packet(const void* data, size_t data_len)
{
	// Ethernetフレーム = ヘッダー(14) + データ
	// 最小フレームサイズは60バイト
	size_t frame_size = 14 + data_len;
	if (frame_size < 60) {
		frame_size = 60; // パディング
	}

	size_t total_size = sizeof(VirtioNetHdr) + frame_size;

	void* tx_buf;
	ALLOC_OR_RETURN_ERROR(tx_buf, total_size, kernel::memory::ALLOC_ZEROED);
	VirtioNetReq* req = reinterpret_cast<VirtioNetReq*>(tx_buf);

	// VirtioNetHdr
	req->net_hdr.flags = 0;
	req->net_hdr.gso_type = NET_HDR_GSO_NONE;
	req->net_hdr.gso_size = 0;
	req->net_hdr.csum_offset = 0;
	req->net_hdr.csum_start = 0;
	req->net_hdr.num_buffers = 0;

	// Ethernetヘッダー (14バイト)
	uint8_t* frame = req->packet_data;

	// 宛先MAC: ブロードキャスト
	memset(frame, 0xFF, 6);

	// 送信元MAC: 自分のMAC
	memcpy(frame + 6, mac_addr, 6);

	// EtherType: 0x9999 (実験用の適当な値)
	frame[12] = 0x99;
	frame[13] = 0x99;

	// データ部分
	memcpy(frame + 14, data, data_len);
	// 残りはゼロパディング (ALLOC_ZEROEDで既に0)

	VirtioEntry chain[1];
	chain[0].addr = (uint64_t)tx_buf;
	chain[0].len = total_size;
	chain[0].write = false;

	push_virtio_entry(tx_queue, chain, 1);

	notify_virtqueue(*net_dev, tx_queue->index);

	return OK;
}

void handle_rx_interrupt(const Message& m)
{
	disable_rx_interrupt();

	VirtioEntry entry_chain[1];
	while (pop_virtio_entry(rx_queue, entry_chain, 1) > 0) {
		VirtioNetReq* req = reinterpret_cast<VirtioNetReq*>(entry_chain[0].addr);
		const size_t packet_len = entry_chain[0].len - sizeof(VirtioNetHdr);

		// Copy the DMA buffer into an OOL buffer whose ownership moves to
		// the NET service (which frees it); the persistent RX buffer goes
		// straight back to the device below
		auto packet_buf =
				kernel::memory::make_kbuf(packet_len, kernel::memory::ALLOC_ZEROED);
		if (packet_buf) {
			memcpy(packet_buf.get(), req->packet_data, packet_len);

			Message msg = {
				.type = MsgType::NET_RX,
				.sender = kernel::task::CURRENT_TASK->id,
			};
			msg.ool.addr = reinterpret_cast<uint64_t>(packet_buf.get());
			msg.ool.size = packet_len;

			if (!IS_ERR(kernel::task::send_message(process_ids::NET, msg))) {
				packet_buf.release();
			}
		} else {
			LOG_ERROR("rx packet dropped: no memory for %lu bytes", packet_len);
		}

		push_virtio_entry(rx_queue, entry_chain, 1);
		notify_virtqueue(*net_dev, rx_queue->index);
	}

	enable_rx_interrupt();
}

void handle_tx_interrupt(const Message& m)
{
	VirtioEntry entry_chain[1];
	while (pop_virtio_entry(tx_queue, entry_chain, 1) > 0) {
		kernel::memory::free(reinterpret_cast<void*>(entry_chain[0].addr));
	}
}

void enable_rx_interrupt()
{
	if (rx_queue == nullptr) {
		LOG_ERROR("RX queue is not initialized");
		return;
	}

	rx_queue->driver->flags &= ~VIRTQ_AVAIL_F_NO_INTERRUPT;
	asm volatile("mfence" ::: "memory");
}

void disable_rx_interrupt()
{
	if (rx_queue == nullptr) {
		LOG_ERROR("RX queue is not initialized");
		return;
	}

	rx_queue->driver->flags |= VIRTQ_AVAIL_F_NO_INTERRUPT;
	asm volatile("mfence" ::: "memory");
}

void handle_transmit_request(const Message& m)
{
	// The frame arrives as an OOL buffer we own; freed on return once it
	// has been copied into the DMA request
	const kernel::memory::unique_kbuf<> frame_buf{ reinterpret_cast<void*>(
			m.ool.addr) };
	if (!frame_buf || m.ool.size == 0 ||
		m.ool.size > sizeof(VirtioNetReq{}.packet_data)) {
		LOG_ERROR("invalid tx frame: %u bytes", m.ool.size);
		return;
	}

	void* tx_buf;
	ALLOC_OR_RETURN(tx_buf, sizeof(VirtioNetReq), kernel::memory::ALLOC_ZEROED);
	VirtioNetReq* req = reinterpret_cast<VirtioNetReq*>(tx_buf);

	req->net_hdr.flags = 0;
	req->net_hdr.gso_type = NET_HDR_GSO_NONE;
	req->net_hdr.gso_size = 0;
	req->net_hdr.csum_offset = 0;
	req->net_hdr.csum_start = 0;
	req->net_hdr.num_buffers = 0;

	memcpy(req->packet_data, frame_buf.get(), m.ool.size);

	VirtioEntry chain[1];
	chain[0].addr = (uint64_t)tx_buf;
	chain[0].len = sizeof(VirtioNetHdr) + m.ool.size;
	chain[0].write = false;

	push_virtio_entry(tx_queue, chain, 1);

	notify_virtqueue(*net_dev, tx_queue->index);
}

void virtio_net_service()
{
	kernel::task::Task* t = kernel::task::CURRENT_TASK;

	init_virtio_net_device();

	// Wire the queue interrupts to this service's doorbells; the interrupt
	// layer itself no longer knows any destination PID
	kernel::interrupt::register_irq_notification(
			kernel::interrupt::InterruptVector::VIRTQUEUE_NET_RX, t->id,
			kernel::task::NotifyType::VIRTIO_NET_RX);
	kernel::interrupt::register_irq_notification(
			kernel::interrupt::InterruptVector::VIRTQUEUE_NET_TX, t->id,
			kernel::task::NotifyType::VIRTIO_NET_TX);

	read_mac_address();
	kernel::net::set_host_mac(mac_addr);

	setup_rx_buffers();

	t->add_msg_handler(MsgType::NOTIFY_VIRTIO_NET_RX, handle_rx_interrupt);
	t->add_msg_handler(MsgType::NOTIFY_VIRTIO_NET_TX, handle_tx_interrupt);
	t->add_msg_handler(MsgType::NET_TX, handle_transmit_request);

	kernel::task::process_messages(t);
}
} // namespace kernel::hw::virtio
