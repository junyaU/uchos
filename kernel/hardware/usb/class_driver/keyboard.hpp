/**
 * @file hardware/usb/class_driver/keyboard.hpp
 *
 * @brief USB keyboard class driver
 *
 * This class represents a USB keyboard driver. It extends from the HID (Human
 * Interface Device) class driver to handle specific keyboard-related
 * functionalities. The class provides mechanisms to subscribe to keyboard events
 * such as key presses and releases, allowing clients to react to these events.
 *
 */

#pragma once

#include "hid.hpp"
#include <array>
#include <functional>

const int KEYBOARD_INTERFACE_PROTOCOL = 1;

namespace kernel::hw::usb
{
class keyboard_driver : public hid_driver
{
public:
	keyboard_driver(device* dev, int interface_index);

	void* operator new(size_t size);
	void operator delete(void* ptr) noexcept;

	void on_data_received() override;

	using observer_type = void(uint8_t modifier, uint8_t keycode, bool pressed);
	void subscribe(std::function<observer_type> observer);
	static std::function<observer_type> default_observer;

private:
	std::array<std::function<observer_type>, 4> observers_;
	int num_observers_ = 0;

	void notify_key_push(uint8_t modifier, uint8_t keycode, bool press);
};
} // namespace kernel::hw::usb