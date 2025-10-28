#include "hardware/virtio/net.hpp"
#include <cstddef>
#include <libs/common/types.hpp>
#include "graphics/log.hpp"
#include "hardware/virtio/pci.hpp"
#include "hardware/virtio/virtio.hpp"
#include "memory/slab.hpp"
#include "task/task.hpp"

namespace
{
void handle_send_packet_request(const Message& m)
{
	LOG_ERROR("send");
	LOG_ERROR("request");
}

void handle_recv_packet_request(const Message& m)
{
	LOG_ERROR("recv");
	LOG_ERROR("request");
}

} // namespace

namespace kernel::hw::virtio
{

VirtioPciDevice* net_dev = nullptr;
uint8_t mac_addr[6] = { 0 };
VirtioVirtqueue* rx_queue = nullptr;
VirtioVirtqueue* tx_queue = nullptr;

error_t setup_rx_buffers()
{
	for (size_t i = 0; i < NUM_RX_BUFFERS; ++i) {
		void* buf;
		ALLOC_OR_RETURN_ERROR(buf, RX_BUFFER_SIZE, kernel::memory::ALLOC_ZEROED);

		rx_queue->desc[i].addr = reinterpret_cast<uint64_t>(buf);
		rx_queue->desc[i].len = RX_BUFFER_SIZE;
		rx_queue->desc[i].flags = VIRTQ_DESC_F_WRITE;

		rx_queue->driver->ring[i] = i;
	}

	rx_queue->driver->index = NUM_RX_BUFFERS;

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

	LOG_INFO("MAC Address: %02x:%02x:%02x:%02x:%02x:%02x", mac_addr[0], mac_addr[1],
			 mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);

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
	ASSERT_OK(init_virtio_pci_device(net_dev, VIRTIO_NET));

	rx_queue = &net_dev->queues[0];
	tx_queue = &net_dev->queues[1];

	kernel::task::CURRENT_TASK->is_initilized = true;

	return OK;
}

struct EthernetFrame {
	uint8_t dst_mac[6];
	uint8_t src_mac[6];
	uint16_t ethertype; // ビッグエンディアン
	uint8_t payload[];
} __attribute__((packed));

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

void virtio_net_service()
{
	kernel::task::Task* t = kernel::task::CURRENT_TASK;

	init_virtio_net_device();

	read_mac_address();

	setup_rx_buffers();

	transmit_packet("Hello, VirtIO Net!", 18);

	t->add_msg_handler(MsgType::IPC_NET_SEND_PACKET, handle_send_packet_request);
	t->add_msg_handler(MsgType::IPC_NET_RECV_PACKET, handle_recv_packet_request);

	kernel::task::process_messages(t);
}
} // namespace kernel::hw::virtio
