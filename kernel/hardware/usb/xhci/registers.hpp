/**
 * @file hardware/usb/xhci/registers.hpp
 *
 */

#pragma once

#include "../../mm_register.hpp"

#include <cstdint>

namespace usb::xhci
{
// hcs = host controller structural

union hcs_params1_register {
	uint32_t data[1];

	struct {
		uint32_t max_device_slots : 8;
		uint32_t max_interrupters : 11;
		uint32_t reserved : 5;
		uint32_t max_ports : 8;

	} __attribute__((packed)) bits;
} __attribute__((packed));

union hcs_params2_register {
	uint32_t data[1];

	struct {
		uint32_t iso_scheduling_threshold : 4;
		uint32_t event_ring_segment_table_max : 4;
		uint32_t reserved : 13;
		uint32_t max_scratchpad_buffers_high : 5;
		uint32_t scratchpad_restore : 1;
		uint32_t max_scratchpad_buffers_low : 5;
	} __attribute__((packed)) bits;
} __attribute__((packed));

union hcs_params3_register {
	uint32_t data[1];

	struct {
		uint32_t u1_device_exit_latency : 8;
		uint32_t reserved : 8;
		uint32_t u2_device_exit_latency : 16;
	} __attribute__((packed)) bits;
} __attribute__((packed));

union hcc_params1_register {
	uint32_t data[1];

	struct {
		uint32_t addressing_capability_64 : 1;
		uint32_t bw_negotiation_capability : 1;
		uint32_t context_size : 1;
		uint32_t port_power_control : 1;
		uint32_t port_indicators : 1;
		uint32_t light_hc_reset_capability : 1;
		uint32_t latency_tolerance_messaging_capability : 1;
		uint32_t no_secondary_sid_support : 1;
		uint32_t parse_all_event_data : 1;
		uint32_t stopped_short_packet_capability : 1;
		uint32_t stopped_edtla_capability : 1;
		uint32_t contiguous_frame_id_capability : 1;
		uint32_t maximum_primary_stream_array_size : 4;
		uint32_t xhci_extended_capabilities_pointer : 16;
	} __attribute__((packed)) bits;
} __attribute__((packed));

union doorbell_offset_register {
	uint32_t data[1];

	struct {
		uint32_t reserved : 2;
		uint32_t doorbell_array_offset : 30;
	} __attribute__((packed)) bits;
} __attribute__((packed));

union doorbell_bitmap {
	uint32_t data[1];

	struct {
		uint32_t db_target : 8;
		uint32_t : 8;
		uint32_t db_stream_id : 16;
	} __attribute__((packed)) bits;
} __attribute__((packed));

class doorbell_register
{
	memory_mapped_register<doorbell_bitmap> reg_;

public:
	void ring(uint8_t target, uint16_t stream_id = 0)
	{
		doorbell_bitmap value{};
		value.bits.db_target = target;
		value.bits.db_stream_id = stream_id;
		reg_.write(value);
	}
};

} // namespace usb::xhci