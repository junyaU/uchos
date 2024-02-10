#include "device_manager.hpp"
#include "../../../graphics/terminal.hpp"
#include "../../../memory/slab.hpp"
#include "../../../types.hpp"

namespace usb::xhci
{
void device_manager::initialize(size_t max_slots)
{
	max_slots_ = max_slots;

	devices_ =
			// NOLINTNEXTLINE(bugprone-sizeof-expression)
			reinterpret_cast<device**>(kmalloc(sizeof(device*) * (max_slots_ + 1),
											   KMALLOC_UNINITIALIZED));
	if (devices_ == nullptr) {
		printk(KERN_ERROR, "failed to allocate memory for devices");
		return;
	}

	contexts_ = reinterpret_cast<device_context**>(
			// NOLINTNEXTLINE(bugprone-sizeof-expression)
			kmalloc(sizeof(device_context*) * (max_slots_ + 1),
					KMALLOC_UNINITIALIZED, 64));
	if (contexts_ == nullptr) {
		kfree(devices_);
		printk(KERN_ERROR, "failed to allocate memory for device contexts");
		return;
	}

	for (size_t i = 0; i <= max_slots_; i++) {
		devices_[i] = nullptr;
		contexts_[i] = nullptr;
	}
}

device_context** device_manager::device_contexts() const { return contexts_; }

device* device_manager::find_by_port(uint8_t port, uint32_t route_string) const
{
	for (size_t i = 0; i <= max_slots_; i++) {
		auto* dev = devices_[i];
		if (dev == nullptr) {
			continue;
		}

		if (dev->context()->slot.bits.root_hub_port_num == port) {
			return dev;
		}
	}

	return nullptr;
}

device* device_manager::find_by_state(enum device::slot_state state) const
{
	for (size_t i = 0; i <= max_slots_; i++) {
		auto* dev = devices_[i];
		if (dev == nullptr) {
			continue;
		}

		if (dev->slot_state() == state) {
			return dev;
		}
	}

	return nullptr;
}

device* device_manager::find_by_slot_id(uint8_t slot_id) const
{
	if (slot_id > max_slots_) {
		return nullptr;
	}

	return devices_[slot_id];
}

void device_manager::allocate_device(uint8_t slot_id,
									 usb::xhci::doorbell_register* dbreg)
{
	if (slot_id > max_slots_) {
		printk(KERN_ERROR, "slot_id %d is out of range", slot_id);
		return;
	}

	if (devices_[slot_id] != nullptr) {
		printk(KERN_ERROR, "slot_id %d is already allocated", slot_id);
		return;
	}

	devices_[slot_id] = reinterpret_cast<device*>(
			kmalloc(sizeof(device), KMALLOC_UNINITIALIZED, 64));
	new (devices_[slot_id]) device(slot_id, dbreg);
}

void device_manager::load_dcbaa(uint8_t slot_id)
{
	if (slot_id > max_slots_) {
		printk(KERN_ERROR, "slot_id %d is out of range", slot_id);
		return;
	}

	auto* dev = devices_[slot_id];
	contexts_[slot_id] = dev->context();
}

void device_manager::remove(uint8_t slot_id)
{
	contexts_[slot_id] = nullptr;
	kfree(devices_[slot_id]);
	devices_[slot_id] = nullptr;
}
} // namespace usb::xhci