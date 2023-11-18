#include "ring.hpp"
#include "../../../graphics/kernel_logger.hpp"
#include "../memory.hpp"

#include <cstring>

namespace usb::xhci
{
ring::~ring()
{
	if (buffer_) {
		free_memory(buffer_);
	}
}

void ring::initialize(size_t buf_size)
{
	if (buffer_) {
		free_memory(buffer_);
	}

	cycle_bit_ = true;
	write_index_ = 0;
	buffer_size_ = buf_size;

	buffer_ = alloc_array<trb>(buffer_size_, 64, 64 * 1024);
	if (buffer_ == nullptr) {
		klogger->error("failed to allocate memory for ring");
		return;
	}

	memset(buffer_, 0, buffer_size_ * sizeof(trb));
}

void ring::copy_to_last(const std::array<uint32_t, 4>& data)
{
	for (int i = 0; i < 3; i++) {
		buffer_[write_index_].data[i] = data[i];
	}

	buffer_[write_index_].data[3] =
			(data[3] & 0xfffffffeU) | static_cast<uint32_t>(cycle_bit_);
}

trb* ring::push(const std::array<uint32_t, 4>& data)
{
	auto trb_ptr = &buffer_[write_index_];
	copy_to_last(data);

	++write_index_;
	if (write_index_ == buffer_size_ - 1) {
		link_trb link{ buffer_ };
		link.bits.toggle_cycle = true;
		copy_to_last(link.data);

		write_index_ = 0;
		cycle_bit_ = !cycle_bit_;
	}

	return trb_ptr;
}

} // namespace usb::xhci