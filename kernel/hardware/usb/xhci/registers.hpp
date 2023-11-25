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

	uint32_t offset() const { return bits.doorbell_array_offset << 2; }
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

// port status and control
union port_sc_bitmap {
	uint32_t data[1];

	struct {
		uint32_t current_connect_status : 1;
		uint32_t port_enabled_disabled : 1;
		uint32_t : 1;
		uint32_t over_current_active : 1;
		uint32_t port_reset : 1;
		uint32_t port_link_state : 4;
		uint32_t port_power : 1;
		uint32_t port_speed : 4;
		uint32_t port_indicator_control : 2;
		uint32_t port_link_state_write_strobe : 1;
		uint32_t connect_status_change : 1;
		uint32_t port_enabled_disabled_change : 1;
		uint32_t warm_port_reset_change : 1;
		uint32_t over_current_change : 1;
		uint32_t port_reset_change : 1;
		uint32_t port_link_state_change : 1;
		uint32_t port_config_error_change : 1;
		uint32_t cold_attach_status : 1;
		uint32_t wake_on_connect_enable : 1;
		uint32_t wake_on_disconnect_enable : 1;
		uint32_t wake_on_over_current_enable : 1;
		uint32_t : 2;
		uint32_t device_removable : 1;
		uint32_t warm_port_reset : 1;
	} __attribute__((packed)) bits;
} __attribute__((packed));

union port_pmsc_bitmap {
	uint32_t data[1];

	struct {
		uint32_t u1_timeout : 8;
		uint32_t u2_timeout : 8;
		uint32_t force_link_pm_accept : 1;
		uint32_t : 15;
	} __attribute__((packed)) bits_usb3;
} __attribute__((packed));

union port_li_bitmap {
	uint32_t data[1];

	struct {
		uint32_t link_error_count : 16;
		uint32_t rx_lane_count : 4;
		uint32_t tx_lane_count : 4;
		uint32_t : 8;
	} __attribute__((packed)) bits_usb3;
} __attribute__((packed));

union port_hlpmc_bitmap {
	uint32_t data[1];

	struct {
		uint32_t host_initiated_resume_duration_mode : 2;
		uint32_t l1_timeout : 8;
		uint32_t best_effort_service_latency_deep : 4;
		uint32_t : 18;
	} __attribute__((packed)) bits_usb2;
} __attribute__((packed));

/**
 * @brief Port registers of xHCI
 *
 * This struct represents the port registers of the xHCI host controller.
 */
struct port_register_set {
	memory_mapped_register<port_sc_bitmap> port_sc;
	memory_mapped_register<port_pmsc_bitmap> port_pmsc;
	memory_mapped_register<port_li_bitmap> port_li;
	memory_mapped_register<port_hlpmc_bitmap> port_hlpmc;
} __attribute__((packed));

using port_register_set_array = array_wrapper<port_register_set>;

union iman_bitmap {
	uint32_t data[1];

	struct {
		uint32_t interrupt_pending : 1;
		uint32_t interrupt_enable : 1;
		uint32_t : 30;
	} __attribute__((packed)) bits;
} __attribute__((packed));

union imod_bitmap {
	uint32_t data[1];

	struct {
		uint32_t interrupt_moderation_interval : 16;
		uint32_t interrupt_moderation_counter : 16;
	} __attribute__((packed)) bits;
} __attribute__((packed));

// event ring segment table size
union erstsz_bitmap {
	uint32_t data[1];

	struct {
		uint32_t event_ring_segment_table_size : 16;
		uint32_t : 16;
	} __attribute__((packed)) bits;

	uint32_t size() const { return bits.event_ring_segment_table_size; }

	void set_size(uint32_t s) { bits.event_ring_segment_table_size = s; }
} __attribute__((packed));

// event ring segment table base address
union erstba_bitmap {
	uint64_t data[1];

	struct {
		uint64_t : 6;
		uint64_t event_ring_segment_table_base_address : 58;
	} __attribute__((packed)) bits;

	uint64_t pointer() const
	{
		return bits.event_ring_segment_table_base_address << 6;
	}

	void set_pointer(uint64_t addr)
	{
		bits.event_ring_segment_table_base_address = addr >> 6;
	}
} __attribute__((packed));

// event ring dequeue pointer
union erdp_bitmap {
	uint64_t data[1];

	struct {
		uint64_t dequeue_erst_segment_index : 3;
		uint64_t event_handler_busy : 1;
		uint64_t event_ring_dequeue_pointer : 60;
	} __attribute__((packed)) bits;

	uint64_t pointer() const { return bits.event_ring_dequeue_pointer << 4; }

	void set_pointer(uint64_t addr) { bits.event_ring_dequeue_pointer = addr >> 4; }
} __attribute__((packed));

/**
 * @brief Interrupter registers of xHCI
 *
 * This struct represents the interrupter registers of the xHCI host controller.
 */
struct interrupter_register_set {
	memory_mapped_register<iman_bitmap> iman;
	memory_mapped_register<imod_bitmap> imod;
	memory_mapped_register<erstsz_bitmap> erstsz;
	uint32_t reserved;
	memory_mapped_register<erstba_bitmap> erstba;
	memory_mapped_register<erdp_bitmap> erdp;
} __attribute__((packed));

using interrupter_register_set_array = array_wrapper<interrupter_register_set>;

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

using doorbell_register_array = array_wrapper<doorbell_register>;

union extended_register_bitmap {
	uint32_t data[1];

	struct {
		uint32_t capability_id : 8;
		uint32_t next_capability_pointer : 8;
		uint32_t value : 16;
	} __attribute__((packed)) bits;
} __attribute__((packed));

class extended_register_list
{
public:
	using value_type = memory_mapped_register<extended_register_bitmap>;

	class iterator
	{
	public:
		iterator(value_type* reg) : reg_(reg) {}
		auto operator->() { return reg_; }
		auto& operator*() { return *reg_; }
		bool operator==(const iterator& rhs) { return reg_ == rhs.reg_; }
		bool operator!=(const iterator& rhs) { return reg_ != rhs.reg_; }
		iterator& operator++();

	private:
		value_type* reg_;
	};

	extended_register_list(uint64_t mmio_base, hcc_params1_register hccp);
	iterator begin() const { return begin_; }
	iterator end() const { return iterator{ nullptr }; }

private:
	const iterator begin_;
};

union usb_legacy_support_bitmap {
	uint32_t data[1];

	struct {
		uint32_t capability_id : 8;
		uint32_t next_pointer : 8;
		uint32_t hc_bios_owned_semaphore : 1;
		uint32_t : 7;
		uint32_t hc_os_owned_semaphore : 1;
		uint32_t : 7;
	} __attribute__((packed)) bits;
} __attribute__((packed));

// void register_command_ring(ring* r, memory_mapped_register<crcr_bitmap>* crcr);

} // namespace usb::xhci