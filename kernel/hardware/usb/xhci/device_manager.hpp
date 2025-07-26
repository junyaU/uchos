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

namespace kernel::hw::usb::xhci
{
class DeviceManager
{
public:
	void initialize(size_t max_slots);
	DeviceContext** device_contexts() const;
	Device* find_by_port(uint8_t port, uint32_t route_string) const;
	Device* find_by_state(enum Device::slot_state state) const;
	Device* find_by_slot_id(uint8_t slot_id) const;
	void allocate_device(uint8_t slot_id, DoorbellRegister* dbreg);
	void load_dcbaa(uint8_t slot_id);
	void remove(uint8_t slot_id);

private:
	// contexts_[0] is reserved for scratchpad buffer array
	DeviceContext** contexts_;
	size_t max_slots_;
	Device** devices_;
};
} // namespace kernel::hw::usb::xhci