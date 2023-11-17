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
};

union setup_stage_trb {
	static const unsigned int TYPE = 2;
	static const unsigned int NO_DATA_STAGE = 0;
	static const unsigned int OUT_DATA_STAGE = 2;
	static const unsigned int IN_DATA_STAGE = 3;

	std::array<uint32_t, 4> data{};

	struct {
		uint32_t request_type : 8;
		uint32_t request : 8;
		uint32_t value : 16;

		uint32_t index : 16;
		uint32_t length : 16;

		uint32_t trb_transfer_length : 17;
		uint32_t : 5;
		uint32_t interrupter_target : 10;

		uint32_t cycle_bit : 1;
		uint32_t : 4;
		uint32_t interrupt_on_completion : 1;
		uint32_t immediate_data : 1;
		uint32_t : 3;
		uint32_t trb_type : 6;
		uint32_t transfer_type : 2;
		uint32_t : 14;
	} __attribute__((packed)) bits;

	setup_stage_trb()
	{
		bits.trb_type = TYPE;
		bits.immediate_data = true;
		bits.trb_transfer_length = 8;
	}
};

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