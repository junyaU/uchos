#include "net/ethernet.hpp"
#include <cstddef>
#include <cstring>
#include <libs/common/types.hpp>
#include "error.hpp"
#include "libs/common/endian.hpp"
#include "libs/common/message.hpp"
#include "log/log.hpp"
#include "memory/slab.hpp"
#include "net/host.hpp"
#include "task/ipc.hpp"
#include "task/task.hpp"

namespace kernel::net
{
error_t transmit_ethernet_frame(const uint8_t* dst_mac,
								kernel::net::EthernetFrameType type,
								const void* payload,
								size_t payload_len)
{
	if (payload_len > ETHERNET_MTU) {
		LOG_ERROR("transmit_ethernet_frame: payload too large: %zu", payload_len);
		return ERR_INVALID_ARG;
	}

	// The frame travels as an OOL buffer whose ownership moves with the
	// message; the NIC service frees it after the DMA copy (issue #314
	// Stage C, Message no longer carries a 1514B inline array)
	const size_t frame_len = sizeof(EthernetFrame) + payload_len;
	auto frame_buf =
			kernel::memory::make_kbuf(frame_len, kernel::memory::ALLOC_ZEROED);
	if (!frame_buf) {
		return ERR_NO_MEMORY;
	}

	auto* frame = static_cast<EthernetFrame*>(frame_buf.get());
	memcpy(frame->dst_mac, dst_mac, MAC_ADDR_SIZE);
	memcpy(frame->src_mac, host_mac(), MAC_ADDR_SIZE);
	frame->ethertype = htons(static_cast<uint16_t>(type));
	memcpy(frame->payload, payload, payload_len);

	Message m = { MsgType::NET_TX, task::CURRENT_TASK->id };
	m.ool.addr = reinterpret_cast<uint64_t>(frame_buf.get());
	m.ool.size = frame_len;

	RETURN_IF_ERROR(task::send_message(process_ids::VIRTIO_NET, m));
	frame_buf.release(); // delivered: the NIC service owns it now

	return OK;
}

} // namespace kernel::net
