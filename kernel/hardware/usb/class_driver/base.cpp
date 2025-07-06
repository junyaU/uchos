#include "base.hpp"
#include "keyboard.hpp"
#include "../descriptor.hpp"

namespace kernel::hw::usb
{
class_driver::class_driver(device* dev) : device_(dev) {}

class_driver::~class_driver() {}

class_driver* new_class_driver(device* dev, const interface_descriptor& if_desc)
{
	const bool is_hid =
			if_desc.interface_class == 3 && if_desc.interface_subclass == 1;

	if (!is_hid) {
		return nullptr;
	}

	switch (if_desc.interface_protocol) {
		case KEYBOARD_INTERFACE_PROTOCOL: {
			auto* driver = new keyboard_driver{ dev, if_desc.interface_number };
			if (keyboard_driver::default_observer) {
				driver->subscribe(keyboard_driver::default_observer);
			}

			return driver;
		}
		default:
			return nullptr;
	}
}

} // namespace kernel::hw::usb
