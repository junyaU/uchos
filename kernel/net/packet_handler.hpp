/**
 * @file net/packet_handler.hpp
 * @brief Network Packet Handler
 *
 * This file defines structures and functions for handling network packets
 * received from VirtIO network device. It implements basic protocol stack
 * including Ethernet, ARP, IP, and ICMP protocols.
 */
#pragma once

namespace kernel::net
{

void packet_handler_service();

} // namespace kernel::net
