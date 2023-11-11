#pragma once

#include <array>
#include <cstdint>

namespace usb::xhci
{
union trb {
	std::array<uint32_t, 4> data{};

	struct {
		uint64_t parameter;
		uint32_t status;
		uint32_t cycle_bit : 1;
		uint32_t evaluate_next_trb : 1;
		uint32_t reserved : 8;
		uint32_t trb_type : 6;
		uint32_t control : 16;
	} __attribute__((packed)) bits;
} __attribute__((packed));

} // namespace usb::xhci