/**
 * @file usb_service.hpp
 * @brief USB service task entry point
 */

#pragma once

namespace kernel::hw
{

/**
 * @brief USB service task: owns PCI/xHCI bring-up and USB event handling
 *
 * Initializes PCI, the xHCI controller and the keyboard observer, then
 * serves NOTIFY_XHCI doorbells in a message loop. Lives in hardware/ so
 * the task layer holds no USB knowledge (issue #315: the scheduler only
 * learns about this service through the boot manifest).
 *
 * @note This function never returns
 */
void usb_handler_service();

} // namespace kernel::hw
