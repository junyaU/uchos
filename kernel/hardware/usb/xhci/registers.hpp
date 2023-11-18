#pragma once

#include <cstdint>

namespace usb::xhci
{
// hcs = host controller structural

union hcs_params1_register {
	uint32_t raw[1];

	struct {
		uint32_t max_device_slots : 8;
		uint32_t max_interrupters : 11;
		uint32_t reserved : 5;
		uint32_t max_ports : 8;

	} __attribute__((packed)) bits;
} __attribute__((packed));

union hcs_params2_register {
	uint32_t raw[1];

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
	uint32_t raw[1];

	struct {
		uint32_t u1_device_exit_latency : 8;
		uint32_t reserved : 8;
		uint32_t u2_device_exit_latency : 16;
	} __attribute__((packed)) bits;
} __attribute__((packed));

union hcc_params1_register {
	uint32_t raw[1];

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
	uint32_t raw[1];

	struct {
		uint32_t reserved : 2;
		uint32_t doorbell_array_offset : 30;
	} __attribute__((packed)) bits;
} __attribute__((packed));

} // namespace usb::xhci