/**
 * @file hardware/usb/xhci/speed.hpp
 *
 * @brief Speed constants for xHCI
 *
 * This file defines constants corresponding to various USB speeds
 * for use with xHCI (eXtensible Host Controller Interface).
 *
 */

#pragma once

namespace kernel::hw::usb::xhci
{
// USB 1.1 Full Speed (12 Mbps)
const int FULL_SPEED = 1;

// USB 1.1 Low Speed (1.5 Mbps)
const int LOW_SPEED = 2;

// USB 2.0 High Speed (480 Mbps)
const int HIGH_SPEED = 3;

// USB 3.0 Super Speed (5 Gbps)
const int SUPER_SPEED = 4;

// USB 3.1 Super Speed Plus (10 Gbps)
const int SUPER_SPEED_PLUS = 5;
} // namespace kernel::hw::usb::xhci