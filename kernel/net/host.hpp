/**
 * @file net/host.hpp
 * @brief Identity of this host on the network
 *
 * The protocol stack owns the host identity; drivers only feed it. The IP
 * address is a static configuration value (UCHos has no DHCP and the QEMU
 * TAP network is fixed), while the MAC address comes from NIC hardware and
 * is registered by the driver during its initialization.
 */

#pragma once

#include <cstdint>
#include "net/ethernet.hpp"

namespace kernel::net
{

/// Statically configured IPv4 address of this host (host byte order),
/// 192.168.100.200 on the QEMU TAP network.
constexpr uint32_t HOST_IP = 0xC0A864C8;

/**
 * @brief Register the MAC address of the NIC this stack transmits through
 *
 * Called once by the NIC driver after it has read the hardware address.
 *
 * @param mac MAC address (MAC_ADDR_SIZE bytes)
 */
void set_host_mac(const uint8_t* mac);

/**
 * @brief MAC address registered by the NIC driver
 *
 * @return Pointer to MAC_ADDR_SIZE bytes; all zeroes until a driver has
 *         registered
 */
const uint8_t* host_mac();

} // namespace kernel::net
