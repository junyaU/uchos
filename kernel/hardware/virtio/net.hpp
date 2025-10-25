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

#include <cstdint>

namespace kernel::hw::virtio
{
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

void virtio_net_service();
} // namespace kernel::hw::virtio