/**
 * @file hardware/usb/xhci/device_manager.hpp
 *
 * @brief Device manager for xHCI
 *
 * This class manages the devices under the xHCI host controller. It is responsible
 * for initializing, tracking, and managing USB devices. It handles device contexts
 * and provides functionality to allocate, find, and remove devices based on various
 * criteria.
 *
 */

#pragma once

#include "context.hpp"
#include "device.hpp"

namespace usb::xhci
{
class device_manager
{
public:
	void initialize(size_t max_slots);
	device_context** device_contexts() const;
	device* find_by_port(uint8_t port, uint32_t route_string) const;
	device* find_by_state(enum device::slot_state state) const;
	device* find_by_slot_id(uint8_t slot_id) const;
	void allocate_device(uint8_t slot_id, doorbell_register* dbreg);
	void load_dcbaa(uint8_t slot_id);
	void remove(uint8_t slot_id);

private:
	device_context** contexts_;
	size_t max_slots_;
	device** devices_;
};
} // namespace usb::xhci