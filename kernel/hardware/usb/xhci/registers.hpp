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

union doorbell_offset_bitmap {
	uint32_t data[1];

	struct {
		uint32_t reserved : 2;
		uint32_t doorbell_array_offset : 30;
	} __attribute__((packed)) bits;
} __attribute__((packed));

union rts_offset_bitmap {
	uint32_t data[1];

	struct {
		uint32_t reserved : 5;
		uint32_t runtime_register_space_offset : 27;
	} __attribute__((packed)) bits;

	uint32_t offset() const { return bits.runtime_register_space_offset << 5; }
} __attribute__((packed));

union hcc_params2_register {
	uint32_t data[1];

	struct {
		uint32_t u3_entry_capability : 1;
		uint32_t
				configure_endpoint_command_max_exit_latency_too_large_capability : 1;
		uint32_t force_save_context_capability : 1;
		uint32_t compliance_transition_capability : 1;
		uint32_t large_esit_payload_capability : 1;
		uint32_t configuration_information_capability : 1;
		uint32_t reserved : 26;
	} __attribute__((packed)) bits;
} __attribute__((packed));

/**
 * @brief Capability registers of xHCI
 *
 * This struct represents the capability registers of the xHCI host controller.
 */
struct capability_registers {
	// Length of the Capability Registers section.
	memory_mapped_register<default_bitmap<uint8_t>> cap_length;

	// Reserved space for future use.
	memory_mapped_register<default_bitmap<uint8_t>> reserved;

	// Version number of the xHCI implementation.
	memory_mapped_register<default_bitmap<uint16_t>> hci_version;

	// Structural parameters of the host controller.
	memory_mapped_register<hcs_params1_register> hcs_params1;
	memory_mapped_register<hcs_params2_register> hcs_params2;
	memory_mapped_register<hcs_params3_register> hcs_params3;

	// Capability parameters of the host controller.
	memory_mapped_register<hcc_params1_register> hcc_params1;

	// Offset to the Doorbell Registers.
	memory_mapped_register<doorbell_offset_bitmap> doorbell_offset;

	// Offset to the Runtime Registers.
	memory_mapped_register<rts_offset_bitmap> rts_offset;

	// Additional capability parameters of the host controller.
	memory_mapped_register<hcc_params2_register> hcc_params2;
};

union usb_cmd_bitmap {
	uint32_t data[1];

	struct {
		uint32_t run_stop : 1;
		uint32_t host_controller_reset : 1;
		uint32_t interrupter_enable : 1;
		uint32_t host_system_error_enable : 1;
		uint32_t : 3;
		uint32_t lignt_host_controller_reset : 1;
		uint32_t controller_save_state : 1;
		uint32_t controller_restore_state : 1;
		uint32_t enable_wrap_event : 1;
		uint32_t enable_u3_mfindex_stop : 1;
		uint32_t stopped_short_packet_enable : 1;
		uint32_t cem_enable : 1;
		uint32_t : 18;
	} __attribute__((packed)) bits;
} __attribute__((packed));

union usb_sts_bitmap {
	uint32_t data[1];

	struct {
		uint32_t host_controller_halted : 1;
		uint32_t : 1;
		uint32_t host_system_error : 1;
		uint32_t event_interrupt : 1;
		uint32_t port_change_detect : 1;
		uint32_t : 3;
		uint32_t save_state_status : 1;
		uint32_t restore_state_status : 1;
		uint32_t save_restore_error : 1;
		uint32_t controller_not_ready : 1;
		uint32_t host_controller_error : 1;
		uint32_t : 19;
	} __attribute__((packed)) bits;
} __attribute__((packed));

union crcr_bitmap {
	uint64_t data[1];

	struct {
		uint64_t ring_cycle_state : 1;
		uint64_t command_stop : 1;
		uint64_t command_abort : 1;
		uint64_t command_ring_running : 1;
		uint64_t : 2;
		uint64_t command_ring_pointer : 58;
	} __attribute__((packed)) bits;

	uint64_t pointer() const { return bits.command_ring_pointer << 6; }

	void set_pointer(uint64_t p) { bits.command_ring_pointer = p >> 6; }
} __attribute__((packed));

union dcbaap_bitmap {
	uint64_t data[1];

	struct {
		uint64_t : 6;
		uint64_t device_context_base_address_array_pointer : 26;
	} __attribute__((packed)) bits;

	uint64_t pointer() const
	{
		return bits.device_context_base_address_array_pointer << 6;
	}

	void set_pointer(uint64_t p)
	{
		bits.device_context_base_address_array_pointer = p >> 6;
	}
} __attribute__((packed));

union config_bitmap {
	uint32_t data[1];

	struct {
		uint32_t max_device_slots_enabled : 8;
		uint32_t u3_entry_enable : 1;
		uint32_t configuration_information_enable : 1;
		uint32_t : 22;
	} __attribute__((packed)) bits;
} __attribute__((packed));

/**
 * @brief Operational registers of xHCI
 *
 * This struct represents the operational registers of the xHCI host controller.
 */
struct operational_registers {
	// The USB Command Register (USB_CMD) is used to issue commands to the xHCI
	// controller, such as run/stop, reset, and other control commands.
	memory_mapped_register<usb_cmd_bitmap> usb_cmd;

	// The USB Status Register (USB_STS) provides status information about the xHCI
	// controller, including error conditions and interrupt status.
	memory_mapped_register<usb_sts_bitmap> usb_sts;

	// The PageSize Register indicates the page size supported by the xHCI
	// controller.
	memory_mapped_register<default_bitmap<uint32_t>> page_size;

	// Reserved memory space for future use or alignment.
	uint32_t reserved1[2];

	// The Device Notification Control Register is used to enable various types of
	// notifications from the controller to the host, such as function wake
	// notifications.
	memory_mapped_register<default_bitmap<uint32_t>> device_notification_ctrl;

	// The Command Ring Control Register (CRCR) is used to manage the command ring,
	// a data structure used for sending commands to the xHCI controller.
	memory_mapped_register<crcr_bitmap> cmd_ring_ctrl;

	// Reserved memory space for future use or alignment.
	uint32_t reserved2[4];

	// The Device Context Base Address Array Pointer (DCBAAP) points to the base
	// address of the Device Context Base Address Array, a key data structure in xHCI
	// that stores the device contexts for connected USB devices.
	memory_mapped_register<dcbaap_bitmap> device_context_base_addr_array_ptr;

	// The Configure Register (CONFIG) is used to configure various parameters of the
	// xHCI controller, including the number of device slots enabled.
	memory_mapped_register<config_bitmap> config;
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