#include "keyboard.hpp"
#include "../memory.hpp"
#include "hid.hpp"

namespace usb
{
keyboard_driver::keyboard_driver(usb::device* dev, int interface_index)
	: hid_driver{ dev, interface_index, 8 }
{
}

void* keyboard_driver::operator new(size_t size)
{
	return alloc_memory(sizeof(keyboard_driver), 0, 0);
}

void keyboard_driver::operator delete(void* ptr) noexcept { free_memory(ptr); }

void keyboard_driver::on_data_received() {}
} // namespace usb