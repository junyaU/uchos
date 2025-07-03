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

/**
 * @brief USB HID interface protocol value for keyboards
 * 
 * According to the USB HID specification, keyboards use protocol value 1.
 */
const int KEYBOARD_INTERFACE_PROTOCOL = 1;

namespace kernel::hw::usb
{
/**
 * @brief USB keyboard driver implementation
 * 
 * This class handles USB keyboards by processing HID reports and
 * notifying observers about key press and release events.
 */
class keyboard_driver : public hid_driver
{
public:
	/**
	 * @brief Construct a new keyboard driver
	 * 
	 * @param dev Parent USB device
	 * @param interface_index Index of the keyboard interface
	 */
	keyboard_driver(device* dev, int interface_index);

	/**
	 * @brief Custom memory allocation for keyboard driver
	 * 
	 * @param size Size to allocate
	 * @return void* Allocated memory
	 */
	void* operator new(size_t size);
	
	/**
	 * @brief Custom memory deallocation for keyboard driver
	 * 
	 * @param ptr Pointer to deallocate
	 */
	void operator delete(void* ptr) noexcept;

	void on_data_received() override;

	/**
	 * @brief Observer function type for keyboard events
	 * 
	 * @param modifier Modifier keys state (Ctrl, Alt, Shift, etc.)
	 * @param keycode HID keycode of the key
	 * @param pressed true if key was pressed, false if released
	 */
	using observer_type = void(uint8_t modifier, uint8_t keycode, bool pressed);
	
	/**
	 * @brief Subscribe to keyboard events
	 * 
	 * @param observer Function to call when keys are pressed/released
	 * 
	 * @note Maximum of 4 observers are supported
	 */
	void subscribe(std::function<observer_type> observer);
	
	/**
	 * @brief Default keyboard event observer
	 * 
	 * This observer is automatically subscribed when a keyboard is connected.
	 */
	static std::function<observer_type> default_observer;

private:
	/**
	 * @brief Array of observer functions
	 */
	std::array<std::function<observer_type>, 4> observers_;
	
	/**
	 * @brief Current number of registered observers
	 */
	int num_observers_ = 0;

	/**
	 * @brief Notify all observers of a key event
	 * 
	 * @param modifier Current modifier keys state
	 * @param keycode HID keycode of the key
	 * @param press true if pressed, false if released
	 */
	void notify_key_push(uint8_t modifier, uint8_t keycode, bool press);
};
} // namespace kernel::hw::usb