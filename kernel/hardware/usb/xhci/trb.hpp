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

union link_trb {
	static const unsigned int TYPE = 6;
	std::array<uint32_t, 4> data{};

	struct {
		uint64_t : 4;
		uint64_t ring_segment_pointer : 60;

		uint32_t : 22;
		uint32_t interrupter_target : 10;

		uint32_t cycle_bit : 1;
		uint32_t toggle_cycle : 1;
		uint32_t : 2;
		uint32_t chain_bit : 1;
		uint32_t interrupt_on_completion : 1;
		uint32_t : 4;
		uint32_t trb_type : 6;
		uint32_t : 16;
	} __attribute__((packed)) bits;

	link_trb(const trb* ring_segment_pointer)
	{
		bits.trb_type = TYPE;
		SetPointer(ring_segment_pointer);
	}

	trb* Pointer() const
	{
		return reinterpret_cast<trb*>(bits.ring_segment_pointer << 4);
	}

	void SetPointer(const trb* p)
	{
		bits.ring_segment_pointer = reinterpret_cast<uint64_t>(p) >> 4;
	}
};

} // namespace usb::xhci