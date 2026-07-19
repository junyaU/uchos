/**
 * @file net/host.cpp
 * @brief Identity of this host on the network
 */

#include "net/host.hpp"
#include <cstring>

namespace
{
uint8_t registered_mac[kernel::net::MAC_ADDR_SIZE] = { 0 };
} // namespace

namespace kernel::net
{

void set_host_mac(const uint8_t* mac) { memcpy(registered_mac, mac, MAC_ADDR_SIZE); }

const uint8_t* host_mac() { return registered_mac; }

} // namespace kernel::net
