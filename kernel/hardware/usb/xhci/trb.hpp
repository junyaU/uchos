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
#include "context.hpp"

#include <array>
#include <cstdint>

namespace kernel::hw::usb::xhci
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

union normal_trb {
	static const unsigned int TYPE = 1;

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
		uint32_t : 2;
		uint32_t block_event_interrupt : 1;
		uint32_t trb_type : 6;
		uint32_t : 16;
	} __attribute__((packed)) bits;

	normal_trb() { bits.trb_type = TYPE; }

	void* pointer() const
	{
		return reinterpret_cast<trb*>(bits.data_buffer_pointer);
	}

	void set_pointer(const void* p)
	{
		bits.data_buffer_pointer = reinterpret_cast<uint64_t>(p);
	}
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

union enable_slot_command_trb {
	static const unsigned int TYPE = 9;
	std::array<uint32_t, 4> data{};

	struct {
		uint32_t : 32;

		uint32_t : 32;

		uint32_t : 32;

		uint32_t cycle_bit : 1;
		uint32_t : 9;
		uint32_t trb_type : 6;
		uint32_t slot_type : 5;
		uint32_t : 11;
	} __attribute__((packed)) bits;

	enable_slot_command_trb() { bits.trb_type = TYPE; }
};

union address_device_command_trb {
	static const unsigned int TYPE = 11;
	std::array<uint32_t, 4> data{};

	struct {
		uint64_t : 4;
		uint64_t InputContext_pointer : 60;

		uint32_t : 32;

		uint32_t cycle_bit : 1;
		uint32_t : 8;
		uint32_t block_set_address_request : 1;
		uint32_t trb_type : 6;
		uint32_t : 8;
		uint32_t slot_id : 8;
	} __attribute__((packed)) bits;

	address_device_command_trb(const InputContext* ctx, uint8_t slot_id)
	{
		bits.trb_type = TYPE;
		bits.slot_id = slot_id;
		set_pointer(ctx);
	}

	InputContext* pointer() const
	{
		return reinterpret_cast<InputContext*>(bits.InputContext_pointer << 4);
	}

	void set_pointer(const InputContext* p)
	{
		bits.InputContext_pointer = reinterpret_cast<uint64_t>(p) >> 4;
	}
};

union configure_endpoint_command_trb {
	static const unsigned int TYPE = 12;
	std::array<uint32_t, 4> data{};

	struct {
		uint64_t : 4;
		uint64_t InputContext_pointer : 60;

		uint32_t : 32;

		uint32_t cycle_bit : 1;
		uint32_t : 8;
		uint32_t deconfigure : 1;
		uint32_t trb_type : 6;
		uint32_t : 8;
		uint32_t slot_id : 8;
	} __attribute__((packed)) bits;

	configure_endpoint_command_trb(const InputContext* ctx, uint8_t slot_id)
	{
		bits.trb_type = TYPE;
		bits.slot_id = slot_id;
		set_pointer(ctx);
	}

	InputContext* pointer() const
	{
		return reinterpret_cast<InputContext*>(bits.InputContext_pointer << 4);
	}

	void set_pointer(const InputContext* p)
	{
		bits.InputContext_pointer = reinterpret_cast<uint64_t>(p) >> 4;
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
		uint32_t EndpointId : 5;
		uint32_t : 3;
		uint32_t slot_id : 8;
	} __attribute__((packed)) bits;

	transfer_event_trb() { bits.trb_type = TYPE; }

	trb* pointer() const { return reinterpret_cast<trb*>(bits.trb_pointer); }

	void set_pointer(const trb* p)
	{
		bits.trb_pointer = reinterpret_cast<uint64_t>(p);
	}

	kernel::hw::usb::EndpointId EndpointId() const
	{
		return kernel::hw::usb::EndpointId{ bits.EndpointId };
	}
};

union command_completion_event_trb {
	static const unsigned int TYPE = 33;
	std::array<uint32_t, 4> data{};

	struct {
		uint64_t : 4;
		uint64_t command_trb_pointer : 60;

		uint32_t command_completion_parameter : 24;
		uint32_t completion_code : 8;

		uint32_t cycle_bit : 1;
		uint32_t : 9;
		uint32_t trb_type : 6;
		uint32_t vf_id : 8;
		uint32_t slot_id : 8;
	} __attribute__((packed)) bits;

	command_completion_event_trb() { bits.trb_type = TYPE; }

	trb* pointer() const
	{
		return reinterpret_cast<trb*>(bits.command_trb_pointer << 4);
	}

	void set_pointer(trb* p)
	{
		bits.command_trb_pointer = reinterpret_cast<uint64_t>(p) >> 4;
	}
};

union port_status_change_event_trb {
	static const unsigned int TYPE = 34;
	std::array<uint32_t, 4> data{};

	struct {
		uint32_t : 24;
		uint32_t port_id : 8;

		uint32_t : 32;

		uint32_t : 24;
		uint32_t completion_code : 8;

		uint32_t cycle_bit : 1;
		uint32_t : 9;
		uint32_t trb_type : 6;
	} __attribute__((packed)) bits;

	port_status_change_event_trb() { bits.trb_type = TYPE; }
};

template<class to_type, class from_type>
to_type* trb_dynamic_cast(from_type* trb)
{
	if (trb->bits.trb_type == to_type::TYPE) {
		return reinterpret_cast<to_type*>(trb);
	}

	return nullptr;
}

setup_stage_trb make_setup_stage_trb(SetupStageData setup_data, int transfer_type);

data_stage_trb make_data_stage_trb(const void* buf, int len, bool dir_in);
} // namespace kernel::hw::usb::xhci