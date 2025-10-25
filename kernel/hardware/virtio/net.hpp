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

namespace kernel::hw::virtio
{
void virtio_net_service();
} // namespace kernel::hw::virtio