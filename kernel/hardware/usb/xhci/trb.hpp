/**
 * @file hardware/usb/xhci/trb.hpp
 *
 * @brief Definitions for various types of Transfer Request Blocks (TRBs) used in
 * xHCI (eXtensible Host Controller Interface).
 *
 * This file contains the union and struct definitions for different types of TRBs,
 * which are the fundamental data structures used in xHCI for managing USB data
 * transfers and events. These TRBs are used to communicate between the host
 * controller and the device, and they are essential for the operation of the USB
 * 3.x system.
 *
 */

#pragma once

#include "../endpoint.hpp"
#include "../setup_stage_data.hpp"

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

union data_stage_trb {
	static const unsigned int TYPE = 3;

	std::array<uint32_t, 4> data{};

	struct {
		uint64_t data_buffer_pointer;

		uint32_t trb_transfer_length : 17;
		uint32_t td_size : 5;
		uint32_t interrupter_target : 10;

		uint32_t cycle_bit : 1;
		uint32_t evaluate_next_trb : 1;
		uint32_t interrupt_on_short_packet : 1;
		uint32_t no_snoop : 1;
		uint32_t chain_bit : 1;
		uint32_t interrupt_on_completion : 1;
		uint32_t immediate_data : 1;
		uint32_t : 3;
		uint32_t trb_type : 6;
		uint32_t direction : 1;
		uint32_t : 15;
	} __attribute__((packed)) bits;

	data_stage_trb() { bits.trb_type = TYPE; }

	void* pointer() const
	{
		return reinterpret_cast<void*>(bits.data_buffer_pointer);
	}

	void set_pointer(const void* p)
	{
		bits.data_buffer_pointer = reinterpret_cast<uint64_t>(p);
	}
};

union status_stage_trb {
	static const unsigned int TYPE = 4;
	std::array<uint32_t, 4> data{};

	struct {
		uint64_t : 64;

		uint32_t : 22;
		uint32_t interrupter_target : 10;

		uint32_t cycle_bit : 1;
		uint32_t evaluate_next_trb : 1;
		uint32_t : 2;
		uint32_t chain_bit : 1;
		uint32_t interrupt_on_completion : 1;
		uint32_t : 4;
		uint32_t trb_type : 6;
		uint32_t direction : 1;
		uint32_t : 15;
	} __attribute__((packed)) bits;

	status_stage_trb() { bits.trb_type = TYPE; }
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

union transfer_event_trb {
	static const unsigned int TYPE = 32;
	std::array<uint32_t, 4> data{};

	struct {
		uint64_t trb_pointer : 64;

		uint32_t trb_transfer_length : 24;
		uint32_t completion_code : 8;

		uint32_t cycle_bit : 1;
		uint32_t : 1;
		uint32_t event_data : 1;
		uint32_t : 7;
		uint32_t trb_type : 6;
		uint32_t endpoint_id : 5;
		uint32_t : 3;
		uint32_t slot_id : 8;
	} __attribute__((packed)) bits;

	transfer_event_trb() { bits.trb_type = TYPE; }

	trb* pointer() const { return reinterpret_cast<trb*>(bits.trb_pointer); }

	void set_pointer(const trb* p)
	{
		bits.trb_pointer = reinterpret_cast<uint64_t>(p);
	}

	endpoint_id endpoint_id() const { return usb::endpoint_id{ bits.endpoint_id }; }
};

template<class to_type, class from_type>
to_type* trb_dynamic_cast(from_type* trb)
{
	if (trb->bits.trb_type == to_type::TYPE) {
		return reinterpret_cast<to_type*>(trb);
	}

	return nullptr;
}

setup_stage_trb make_setup_stage_trb(setup_stage_data setup_data, int transfer_type);

data_stage_trb make_data_stage_trb(const void* buf, int len, bool dir_in);
} // namespace usb::xhci