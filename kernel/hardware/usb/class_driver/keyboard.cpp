#include "keyboard.hpp"
#include "../device.hpp"
#include "hid.hpp"
#include "memory/slab.hpp"
#include <cstddef>
#include <cstdint>
#include <functional>
#include <utility>

#include <bitset>

namespace kernel::hw::usb
{
KeyboardDriver::KeyboardDriver(usb::Device* dev, int interface_index)
    : HidDriver{ dev, interface_index, 8 }
{
}

void* KeyboardDriver::operator new(size_t size)
{
	return kernel::memory::alloc(sizeof(KeyboardDriver), kernel::memory::ALLOC_UNINITIALIZED);
}

void KeyboardDriver::operator delete(void* ptr) noexcept
{
	kernel::memory::free(ptr);
}

void KeyboardDriver::on_data_received()
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

void KeyboardDriver::subscribe(std::function<observer_type> o)
{
	if (num_observers_ < observers_.size()) {
		observers_[num_observers_++] = std::move(o);
	}
}

std::function<KeyboardDriver::observer_type> KeyboardDriver::default_observer;

void KeyboardDriver::notify_key_push(uint8_t modifier, uint8_t keycode, bool press)
{
	for (int i = 0; i < num_observers_; i++) {
		observers_[i](modifier, keycode, press);
	}
}

}  // namespace kernel::hw::usb