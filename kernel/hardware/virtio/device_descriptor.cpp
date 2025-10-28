/**
 * @file device_descriptor.cpp
 * @brief VirtIO device descriptor definitions and search function
 *
 * This file contains the definitions of all supported VirtIO devices
 * and their interrupt configurations.
 * @date 2025-01-28
 */
#include "device_descriptor.hpp"

#include "../../interrupt/handlers.hpp"
#include "../../interrupt/vector.hpp"
#include "virtio.hpp"

namespace kernel::hw::virtio
{

/**
 * @brief Array of all supported VirtIO device descriptors
 *
 * Each entry defines a VirtIO device with its interrupt configurations.
 * To add a new device, simply add a new entry to this array.
 */
constexpr VirtioDeviceDescriptor VIRTIO_DEVICE_DESCRIPTORS[] = {
	{
			.device_id = VIRTIO_BLK,
			.name = "virtio-blk",
			.num_interrupts = 2,
			.interrupts =
					{
							{
									.vector = interrupt::InterruptVector::VIRTIO_BLK,
									.name = "config",
									.handler = interrupt::on_virtio_blk_interrupt,
							},
							{
									.vector = interrupt::InterruptVector::VIRTQUEUE_BLK,
									.name = "queue",
									.handler = interrupt::on_virtio_blk_queue_interrupt,
							},
					},
	},
	{
			.device_id = VIRTIO_NET,
			.name = "virtio-net",
			.num_interrupts = 3,
			.interrupts =
					{
							{
									.vector = interrupt::InterruptVector::VIRTIO_NET,
									.name = "config",
									.handler = interrupt::on_virtio_net_interrupt,
							},
							{
									.vector = interrupt::InterruptVector::VIRTQUEUE_NET_RX,
									.name = "rx_queue",
									.handler = interrupt::on_virtio_net_rx_queue_interrupt,
							},
							{
									.vector = interrupt::InterruptVector::VIRTQUEUE_NET_TX,
									.name = "tx_queue",
									.handler = interrupt::on_virtio_net_tx_queue_interrupt,
							},
					},
	},
};

constexpr size_t NUM_VIRTIO_DEVICE_DESCRIPTORS =
		sizeof(VIRTIO_DEVICE_DESCRIPTORS) / sizeof(VIRTIO_DEVICE_DESCRIPTORS[0]);

const VirtioDeviceDescriptor* find_virtio_device_descriptor(int device_type)
{
	for (size_t i = 0; i < NUM_VIRTIO_DEVICE_DESCRIPTORS; ++i) {
		if (VIRTIO_DEVICE_DESCRIPTORS[i].device_id ==
			static_cast<uint16_t>(device_type)) {
			return &VIRTIO_DEVICE_DESCRIPTORS[i];
		}
	}
	return nullptr;
}

} // namespace kernel::hw::virtio
