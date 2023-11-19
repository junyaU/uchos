#include "base.hpp"
#include "keyboard.hpp"

namespace usb
{
class_driver::class_driver(device* dev) : device_(dev) {}

class_driver::~class_driver() {}

class_driver* new_class_driver(device* dev, const interface_descriptor& if_desc)
{
	bool is_hid = if_desc.interface_class == 3 & if_desc.interface_subclass == 1;

	if (!is_hid) {
		return nullptr;
	}

	switch (if_desc.interface_protocol) {
		case KEYBOARD_INTERFACE_PROTOCOL:
			return new keyboard_driver{ dev, if_desc.interface_number };
	}

	return nullptr;
}

} // namespace usb
