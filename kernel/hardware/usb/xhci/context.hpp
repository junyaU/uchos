/**
 * @file hardware/usb/xhci/context.hpp
 *
 * @brief USB xHCI context
 *
 * This file contains structures and unions used to represent and manage
 * contexts in USB xHCI (eXtensible Host Controller Interface). These contexts
 * include slot, endpoint, and various control contexts that are essential
 * for managing the state and configuration of USB devices and their
 * respective endpoints.
 *
 */

#pragma once

#include "../endpoint.hpp"
#include "trb.hpp"

#include <cstdint>

namespace usb::xhci
{

union slot_context {
	uint32_t raw[8];

	struct {
		uint32_t route_string : 20;
		uint32_t speed : 4;
		uint32_t reserved : 1;
		// multi transaction translator
		uint32_t mtt : 1;
		uint32_t hub : 1;
		uint32_t context_entries : 5;

		uint32_t max_exit_latency : 16;
		uint32_t root_hub_port_num : 8;
		uint32_t num_ports : 8;

		// TT : Transaction Translator
		uint32_t tt_hub_slot_id : 8;
		uint32_t tt_port_num : 8;
		uint32_t ttt : 2;
		uint32_t reserved2 : 4;
		uint32_t interrupter_target : 10;

		uint32_t usb_device_address : 8;
		uint32_t : 19;
		uint32_t slot_state : 5;
	} __attribute__((packed)) bits;
} __attribute__((packed));

union endpoint_context {
	uint32_t raw[8];

	struct {
		uint32_t ep_state : 3;
		uint32_t : 5;
		uint32_t mult : 2;
		uint32_t max_primary_streams : 5;
		uint32_t linear_stream_array : 1;
		uint32_t interval : 8;
		uint32_t max_esit_payload_hi : 8;

		uint32_t : 1;
		uint32_t error_count : 2;
		uint32_t ep_type : 3;
		uint32_t : 1;
		uint32_t host_initiate_disable : 1;
		uint32_t max_burst_size : 8;
		uint32_t max_packet_size : 16;

		uint32_t dequeue_cycle_state : 1;
		uint32_t : 3;
		uint64_t tr_dequeue_pointer : 60;

		uint32_t average_trb_length : 16;
		uint32_t max_esit_payload_lo : 16;
	} __attribute__((packed)) bits;

	trb* transfer_ring_buffer() const
	{
		return reinterpret_cast<trb*>(bits.tr_dequeue_pointer << 4);
	};

	void set_transfer_ring_buffer(trb* trb)
	{
		bits.tr_dequeue_pointer = reinterpret_cast<uint64_t>(trb) >> 4;
	};
} __attribute__((packed));

// identify endpoint context index
struct device_context_index {
	int value;

	explicit device_context_index(int value) : value(value) {}
	device_context_index(endpoint_id id) : value(id.address()) {}
	device_context_index(int endpoint_num, bool in)
		: value{ 2 * endpoint_num + (endpoint_num == 0 ? 1 : in) }
	{
	}

	device_context_index(const device_context_index& index) = default;
	device_context_index& operator=(const device_context_index& index) = default;
};

struct device_context {
	slot_context slot;
	endpoint_context endpoints[31];
} __attribute__((packed));

struct input_control_context {
	uint32_t drop_context_flags;
	uint32_t add_context_flags;
	uint32_t reserved1[5];
	uint8_t configuration_value;
	uint8_t interface_number;
	uint8_t alternate_setting;
	uint8_t reserved2;
} __attribute__((packed));

struct input_context {
	input_control_context control;
	slot_context slot;
	endpoint_context endpoints[31];

	slot_context* enable_slot_context()
	{
		control.add_context_flags |= 1;
		return &slot;
	}

	endpoint_context* enable_endpoint_context(device_context_index index)
	{
		control.add_context_flags |= 1U << index.value;
		return &endpoints[index.value - 1];
	}

} __attribute__((packed));

} // namespace usb::xhci