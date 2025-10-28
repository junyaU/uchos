/**
 * @file hardware/virtio/net.hpp
 *
 * @brief VirtIO Network Device Driver
 *
 * This file defines the structures, functions, and constants used for
 * interacting with VirtIO network devices. It provides functionalities for
 * sending and receiving network packets, as well as initializing the
 * network device.
 */
#pragma once

#include <cstddef>
#include <cstdint>

namespace kernel::hw::virtio
{
// VirtIO Network Device Features
constexpr uint64_t VIRTIO_NET_F_CSUM = (1ULL << 0);
constexpr uint64_t VIRTIO_NET_F_GUEST_CSUM = (1ULL << 1);
constexpr uint64_t VIRTIO_NET_F_MAC = (1ULL << 5);
constexpr uint64_t VIRTIO_NET_F_GSO = (1ULL << 6);
constexpr uint64_t VIRTIO_NET_F_GUEST_TSO4 = (1ULL << 7);
constexpr uint64_t VIRTIO_NET_F_GUEST_TSO6 = (1ULL << 8);
constexpr uint64_t VIRTIO_NET_F_HOST_TSO4 = (1ULL << 11);
constexpr uint64_t VIRTIO_NET_F_HOST_TSO6 = (1ULL << 12);
constexpr uint64_t VIRTIO_NET_F_MRG_RXBUF = (1ULL << 15);
constexpr uint64_t VIRTIO_NET_F_STATUS = (1ULL << 16);
constexpr uint64_t VIRTIO_NET_F_CTRL_VQ = (1ULL << 17);
constexpr uint64_t VIRTIO_NET_F_MQ = (1ULL << 22);

constexpr size_t RX_BUFFER_SIZE = 2048;
constexpr size_t NUM_RX_BUFFERS = 32;
constexpr size_t MAX_PACKET_SIZE = 1514;

struct VirtioNetConfig {
	uint8_t mac[6];
	uint16_t status;
	uint16_t max_virtqueue_pairs;
	uint16_t mtu;
	uint32_t speed;
	uint8_t duplex;
	uint8_t rss_max_key_size;
	uint16_t rss_max_indirection_table_length;
	uint32_t supported_hash_types;
	uint32_t supported_tunnel_types;
} __attribute__((packed));

constexpr size_t NET_HDR_GSO_NONE = 0;

struct VirtioNetHdr {
	uint8_t flags;
	uint8_t gso_type;
	uint16_t hdr_len;
	uint16_t gso_size;
	uint16_t csum_start;
	uint16_t csum_offset;
	uint16_t num_buffers;
} __attribute__((packed));

struct VirtioNetReq {
	VirtioNetHdr net_hdr;
	uint8_t packet_data[MAX_PACKET_SIZE];
	// Followed by packet data
} __attribute__((packed));

void virtio_net_service();
} // namespace kernel::hw::virtio
