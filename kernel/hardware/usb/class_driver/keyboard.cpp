#include "keyboard.hpp"
#include "hid.hpp"
#include "memory/slab.hpp"
#include <libs/common/types.hpp>

#include <bitset>

namespace kernel::hw::usb
{
keyboard_driver::keyboard_driver(usb::device* dev, int interface_index)
	: hid_driver{ dev, interface_index, 8 }
{
}

void* keyboard_driver::operator new(size_t size)
{
	return kernel::memory::alloc(sizeof(keyboard_driver), kernel::memory::ALLOC_UNINITIALIZED);
}

void keyboard_driver::operator delete(void* ptr) noexcept { kernel::memory::free(ptr); }

void keyboard_driver::on_data_received()
{
	const int keycode_num = 256;

	std::bitset<keycode_num> prev, current;
	for (int i = 2; i < 8; i++) {
		prev.set(prev_buffer()[i], true);
		current.set(buffer()[i], true);
	}

	const auto changed = prev ^ current;
	const auto pressed = changed & current;

	for (int key = 1; key < keycode_num; key++) {
		if (changed.test(key)) {
			notify_key_push(buffer()[0], key, pressed.test(key));
		}
	}
}

void keyboard_driver::subscribe(std::function<observer_type> o)
{
	if (num_observers_ < observers_.size()) {
		observers_[num_observers_++] = std::move(o);
	}
}

std::function<keyboard_driver::observer_type> keyboard_driver::default_observer;

void keyboard_driver::notify_key_push(uint8_t modifier, uint8_t keycode, bool press)
{
	for (int i = 0; i < num_observers_; i++) {
		observers_[i](modifier, keycode, press);
	}
}

} // namespace kernel::hw::usb