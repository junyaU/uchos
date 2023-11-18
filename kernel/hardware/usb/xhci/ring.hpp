#pragma once

#include "registers.hpp"
#include "trb.hpp"

#include <cstdint>
#include <vector>

namespace usb::xhci
{
// Command/Transfer ring
class ring
{
public:
	ring() = default;
	ring(const ring&) = delete;
	~ring();
	ring& operator=(const ring&) = delete;

	void initialize(size_t buf_size);

	template<class trb_type>
	trb* push(const trb_type& trb)
	{
		return push(trb.data());
	}

	trb* buffer() const { return buffer_; }

private:
	trb* buffer_ = nullptr;
	size_t buffer_size_ = 0;

	bool cycle_bit_;
	size_t write_index_;

	void copy_to_last(const std::array<uint32_t, 4>& data);

	trb* push(const std::array<uint32_t, 4>& data);
};

} // namespace usb::xhci