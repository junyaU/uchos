#include "net/ethernet.hpp"
#include <cstddef>
#include <libs/common/types.hpp>
#include "hardware/virtio/net.hpp"
#include "libs/common/endian.hpp"
#include "libs/common/message.hpp"
#include "memory/slab.hpp"
#include "net/arp.hpp"
#include "task/ipc.hpp"
#include "task/task.hpp"

namespace kernel::net
{
error_t transmit_ethernet_frame(const uint8_t* dst_mac,
								kernel::net::EthernetFrameType type,
								const void* payload,
								size_t payload_len)
{
	void* frame_buf;
	ALLOC_OR_RETURN_ERROR(frame_buf, sizeof(EthernetFrame),
						  kernel::memory::ALLOC_ZEROED);

	EthernetFrame* frame = reinterpret_cast<EthernetFrame*>(frame_buf);
	memcpy(frame->dst_mac, dst_mac, MAC_ADDR_SIZE);
	memcpy(frame->src_mac, hw::virtio::mac_addr, MAC_ADDR_SIZE);
	frame->ethertype = htons(static_cast<uint16_t>(type));

	memcpy(frame->payload, payload, payload_len);

	Message m = { MsgType::IPC_TRANSMIT_TO_NIC, task::CURRENT_TASK->id };

	task::send_message(process_ids::VIRTIO_NET, m);

	return OK;
}

} // namespace kernel::net
