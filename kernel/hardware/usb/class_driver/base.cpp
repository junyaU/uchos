#include "base.hpp"
#include "../descriptor.hpp"
#include "keyboard.hpp"

namespace kernel::hw::usb
{
ClassDriver::ClassDriver(Device* dev) : device_(dev) {}

ClassDriver::~ClassDriver() {}

ClassDriver* new_class_driver(Device* dev, const InterfaceDescriptor& if_desc)
{
	const bool is_hid =
			if_desc.interface_class == 3 && if_desc.interface_subclass == 1;

	if (!is_hid) {
		return nullptr;
	}

	switch (if_desc.interface_protocol) {
		case KEYBOARD_INTERFACE_PROTOCOL: {
			auto* driver = new KeyboardDriver{ dev, if_desc.interface_number };
			if (KeyboardDriver::default_observer) {
				driver->subscribe(KeyboardDriver::default_observer);
			}

			return driver;
		}
		default:
			return nullptr;
	}
}

} // namespace kernel::hw::usb
