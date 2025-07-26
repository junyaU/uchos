#include "xhci.hpp"
#include <cstdint>
#include <cstring>
#include <algorithm>
#include "../../pci.hpp"
#include "../../mm_register.hpp"
#include "asm_utils.h"
#include "context.hpp"
#include "graphics/log.hpp"
#include "interrupt/vector.hpp"
#include "memory/slab.hpp"
#include "port.hpp"
#include "registers.hpp"
#include "ring.hpp"
#include "speed.hpp"
#include "trb.hpp"
#include "../endpoint.hpp"

namespace
{
using namespace kernel::hw::usb::xhci;

unsigned int determine_max_packet_size_for_control_pipe(int port_speed)
{
	switch (port_speed) {
		case SUPER_SPEED:
			return 512;
		case HIGH_SPEED:
			return 64;
		default:
			return 8;
	}
}

void enable_slot(Controller& xhc, Port& p)
{
	const bool is_enabled = p.is_enabled();
	const bool reset_completed = p.is_port_reset_changed();

	if (is_enabled && reset_completed) {
		p.clear_port_reset_changed();
		port_connection_states[p.number()] = PortConnectionState::ENABLEING_SLOT;

		const enable_slot_command_trb cmd{};
		xhc.command_ring()->push(cmd);
		xhc.doorbell_register_at(0)->ring(0);
	}
}

void initialize_device(Controller& xhc, uint8_t port_id, uint8_t slot_id)
{
	auto* dev = xhc.device_manager()->find_by_slot_id(slot_id);
	if (dev == nullptr) {
		LOG_ERROR("device not found for slot id %d", slot_id);
		return;
	}

	port_connection_states[port_id] = PortConnectionState::INITIALIZING_DEVICE;
	dev->start_initialize();
}

void complete_configuration(Controller& xhc, uint8_t port_id, uint8_t slot_id)
{
	auto* dev = xhc.device_manager()->find_by_slot_id(slot_id);
	if (dev == nullptr) {
		LOG_ERROR("device not found for slot id %d", slot_id);
		return;
	}

	dev->on_endpoints_configured();

	port_connection_states[port_id] = PortConnectionState::CONFIGURED;
}

void address_device(Controller& xhc, uint8_t port_id, uint8_t slot_id)
{
	xhc.device_manager()->allocate_device(slot_id,
										  xhc.doorbell_register_at(slot_id));

	Device* dev = xhc.device_manager()->find_by_slot_id(slot_id);
	if (dev == nullptr) {
		LOG_ERROR("device not found for slot id %d", slot_id);
		return;
	}

	memset(&dev->input_context()->control, 0, sizeof(InputControlContext));

	const auto ep0_dci = DeviceContextIndex{ 0, false };
	auto* slot_ctx = dev->input_context()->enable_slot_context();
	auto* ep0_ctx = dev->input_context()->enable_endpoint_context(ep0_dci);

	auto port = xhc.port_at(port_id);
	initialize_slot_context(*slot_ctx, port_id, port.speed());

	initialize_endpoint0_context(
			*ep0_ctx, dev->alloc_transfer_ring(ep0_dci, 32)->buffer(),
			determine_max_packet_size_for_control_pipe(slot_ctx->bits.speed));

	xhc.device_manager()->load_dcbaa(slot_id);

	port_connection_states[port_id] = PortConnectionState::ADDRESSING_DEVICE;

	const address_device_command_trb cmd{ dev->input_context(), slot_id };
	xhc.command_ring()->push(cmd);
	xhc.doorbell_register_at(0)->ring(0);
}

void on_event(Controller& xhc, port_status_change_event_trb& trb)
{
	auto port_id = trb.bits.port_id;
	auto p = xhc.port_at(port_id);

	switch (port_connection_states[port_id]) {
		case PortConnectionState::DISCONNECTED:
			reset_port(p);
			break;
		case PortConnectionState::RESETTING_PORT:
			enable_slot(xhc, p);
			break;
		default:
			LOG_ERROR("port %d is not disconnected or resetting", port_id);
			break;
	}
}

void on_event(Controller& xhc, transfer_event_trb& trb)
{
	const uint8_t slot_id = trb.bits.slot_id;
	auto* dev = xhc.device_manager()->find_by_slot_id(slot_id);
	if (dev == nullptr) {
		LOG_ERROR("device not found for slot id %d", slot_id);
		return;
	}

	dev->on_transfer_event_received(trb);

	const auto port_id = dev->context()->slot.bits.root_hub_port_num;
	if (dev->is_initialized() &&
		port_connection_states[port_id] ==
				PortConnectionState::INITIALIZING_DEVICE) {
		configure_endpoints(xhc, *dev);
	}
}

void on_event(Controller& xhc, command_completion_event_trb& trb)
{
	const auto issuer_type = trb.pointer()->bits.trb_type;
	const auto slot_id = trb.bits.slot_id;

	switch (issuer_type) {
		case enable_slot_command_trb::TYPE:
			if (port_connection_states[addressing_port] !=
				PortConnectionState::ENABLEING_SLOT) {
				LOG_ERROR("port %d is not enabling slot", addressing_port);
				break;
			}

			address_device(xhc, addressing_port, slot_id);
			break;

		case address_device_command_trb::TYPE: {
			auto* dev = xhc.device_manager()->find_by_slot_id(slot_id);
			if (dev == nullptr) {
				LOG_ERROR("device not found for slot id %d", slot_id);
				break;
			};

			auto port_id = dev->context()->slot.bits.root_hub_port_num;
			if (port_id != addressing_port) {
				LOG_ERROR("port id %d is not equal to addressing port %d", port_id,
						  addressing_port);
				break;
			}

			if (port_connection_states[port_id] !=
				PortConnectionState::ADDRESSING_DEVICE) {
				LOG_ERROR("port %d is not addressing device", port_id);
				break;
			}

			addressing_port = 0;
			for (int i = 0; i < port_connection_states.size(); i++) {
				if (port_connection_states[i] ==
					PortConnectionState::WAITING_ADDRESSED) {
					auto p = xhc.port_at(i);
					reset_port(p);
					break;
				}
			}

			initialize_device(xhc, port_id, slot_id);
			break;
		}

		case configure_endpoint_command_trb::TYPE: {
			auto* dev = xhc.device_manager()->find_by_slot_id(slot_id);
			if (dev == nullptr) {
				LOG_ERROR("device not found for slot id %d", slot_id);
				break;
			}

			auto port_id = dev->context()->slot.bits.root_hub_port_num;
			if (port_connection_states[port_id] !=
				PortConnectionState::CONFIGURING_ENDPOINTS) {
				LOG_ERROR("port %d is not configuring endpoints", port_id);
				break;
			}

			complete_configuration(xhc, port_id, slot_id);
			break;
		}

		default:
			LOG_ERROR("Unknown command completion event type: %d", issuer_type);
			break;
	}
}

void request_hc_ownership(uintptr_t mmio_base, hcc_params1_register hccp)
{
	const ExtendedRegisterList extended_regs{ mmio_base, hccp };

	auto ext_usb_legacy_support =
			std::find_if(extended_regs.begin(), extended_regs.end(), [](auto& reg) {
				return reg.read().bits.capability_id == 1;
			});
	if (ext_usb_legacy_support == extended_regs.end()) {
		return;
	}

	auto& reg = reinterpret_cast<MemoryMappedRegister<usb_legacy_support_bitmap>&>(
			*ext_usb_legacy_support);
	auto r = reg.read();
	if (r.bits.hc_os_owned_semaphore) {
		return;
	}

	r.bits.hc_os_owned_semaphore = 1;
	reg.write(r);

	do {
		r = reg.read();
	} while (r.bits.hc_os_owned_semaphore == 0);
}
} // namespace

namespace kernel::hw::usb::xhci
{
Controller::Controller(uintptr_t mmio_base)
	: mmio_base_(mmio_base),
	  cap_regs_(reinterpret_cast<CapabilityRegisters*>(mmio_base)),
	  op_regs_(reinterpret_cast<OperationalRegisters*>(
			  mmio_base + cap_regs_->cap_length.read())),
	  max_ports_(static_cast<uint8_t>(cap_regs_->hcs_params1.read().bits.max_ports))
{
}

void Controller::initialize()
{
	device_manager_.initialize(DEVICE_SIZE);

	request_hc_ownership(mmio_base_, cap_regs_->hcc_params1.read());

	// Stop the controller for initialization
	auto usb_command = op_regs_->usb_cmd.read();
	usb_command.bits.interrupter_enable = false;
	usb_command.bits.host_system_error_enable = false;
	usb_command.bits.enable_wrap_event = false;
	if (!op_regs_->usb_sts.read().bits.host_controller_halted) {
		usb_command.bits.run_stop = false;
	}

	op_regs_->usb_cmd.write(usb_command);
	while (!op_regs_->usb_sts.read().bits.host_controller_halted) {
	}

	// Reset controller
	usb_command = op_regs_->usb_cmd.read();
	usb_command.bits.host_controller_reset = true;
	op_regs_->usb_cmd.write(usb_command);
	while (op_regs_->usb_cmd.read().bits.host_controller_reset) {
	}
	while (op_regs_->usb_sts.read().bits.controller_not_ready) {
	}

	auto config = op_regs_->config.read();
	config.bits.max_device_slots_enabled = DEVICE_SIZE;
	op_regs_->config.write(config);

	auto hcs_params2 = cap_regs_->hcs_params2.read();
	const uint8_t max_scratchpad_buffers =
			hcs_params2.bits.max_scratchpad_buffers_low |
			(hcs_params2.bits.max_scratchpad_buffers_high << 5);

	if (max_scratchpad_buffers > 0) {
		void* scratchpad_buf_arr_ptr;
		ALLOC_OR_RETURN(scratchpad_buf_arr_ptr, sizeof(void*) * max_scratchpad_buffers, kernel::memory::ALLOC_UNINITIALIZED);
		auto* scratchpad_buf_arr = reinterpret_cast<void**>(scratchpad_buf_arr_ptr);

		for (int i = 0; i < max_scratchpad_buffers; i++) {
			scratchpad_buf_arr[i] = kernel::memory::alloc(4096, kernel::memory::ALLOC_UNINITIALIZED);
			if (scratchpad_buf_arr[i] == nullptr) {
				LOG_ERROR("Memory allocation failed: scratchpad_buf_arr[%d] (size=4096)", i);
				return;
			}
		}

		device_manager_.device_contexts()[0] =
				reinterpret_cast<DeviceContext*>(scratchpad_buf_arr);
	}

	dcbaap_bitmap device_context_base_address_array_pointer = {};
	device_context_base_address_array_pointer.set_pointer(
			reinterpret_cast<uint64_t>(device_manager_.device_contexts()));
	op_regs_->device_context_base_addr_array_ptr.write(
			device_context_base_address_array_pointer);

	auto* primary_interrupter = &interrupter_register_sets()[0];
	command_ring_.initialize(32);
	register_command_ring(&command_ring_, &op_regs_->cmd_ring_ctrl);
	event_ring_.initialize(32, primary_interrupter);

	auto interrupt_management = primary_interrupter->iman.read();
	interrupt_management.bits.interrupt_pending = true;
	interrupt_management.bits.interrupt_enable = true;
	primary_interrupter->iman.write(interrupt_management);

	usb_command = op_regs_->usb_cmd.read();
	usb_command.bits.interrupter_enable = true;
	op_regs_->usb_cmd.write(usb_command);
}

void Controller::run()
{
	auto usb_command = op_regs_->usb_cmd.read();
	usb_command.bits.run_stop = true;
	op_regs_->usb_cmd.write(usb_command);

	while (op_regs_->usb_sts.read().bits.host_controller_halted) {
	}
}

DoorbellRegister* Controller::doorbell_register_at(uint8_t index)
{
	return &doorbell_registers()[index];
}

void configure_port(Controller& xhc, Port& p)
{
	if (port_connection_states[p.number()] == PortConnectionState::DISCONNECTED) {
		reset_port(p);
	}
}

void configure_endpoints(Controller& xhc, Device& dev)
{
	const auto* configs = dev.EndpointConfigs();
	const auto len = dev.num_EndpointConfigs();

	memset(&dev.input_context()->control, 0, sizeof(InputControlContext));
	memcpy(&dev.input_context()->slot, &dev.context()->slot, sizeof(slot_context));

	auto* slot_ctx = dev.input_context()->enable_slot_context();
	// setting endpoint num
	slot_ctx->bits.context_entries = 31;
	const auto port_id{ dev.context()->slot.bits.root_hub_port_num };
	const int port_speed = xhc.port_at(port_id).speed();
	if (port_speed == 0 || port_speed > SUPER_SPEED_PLUS) {
		LOG_ERROR("unsupported port speed");
		return;
	}

	auto convert_interval = [port_speed](EndpointType type, int interval) {
		if (port_speed == FULL_SPEED || port_speed == LOW_SPEED) {
			if (type == EndpointType::ISOCHRONOUS) {
				return interval + 2;
			}
			return most_significant_bit(interval) + 3;
		}

		return interval - 1;
	};

	for (int i = 0; i < len; i++) {
		const DeviceContextIndex ep_dci{ configs[i].id };
		auto* ep_ctx = dev.input_context()->enable_endpoint_context(ep_dci);
		switch (configs[i].type) {
			case EndpointType::CONTROL:
				ep_ctx->bits.ep_type = 4;
				break;
			case EndpointType::ISOCHRONOUS:
				ep_ctx->bits.ep_type = configs[i].id.is_in() ? 5 : 1;
				break;
			case EndpointType::BULK:
				ep_ctx->bits.ep_type = configs[i].id.is_in() ? 6 : 2;
				break;
			case EndpointType::INTERRUPT:
				ep_ctx->bits.ep_type = configs[i].id.is_in() ? 7 : 3;
				break;
		}

		ep_ctx->bits.max_packet_size = configs[i].max_packet_size;
		ep_ctx->bits.interval =
				convert_interval(configs[i].type, configs[i].interval);
		ep_ctx->bits.average_trb_length = 1;

		auto* transfer_ring = dev.alloc_transfer_ring(ep_dci, 32);
		ep_ctx->set_transfer_ring_buffer(transfer_ring->buffer());

		ep_ctx->bits.dequeue_cycle_state = 1;
		ep_ctx->bits.max_primary_streams = 0;
		ep_ctx->bits.mult = 0;
		ep_ctx->bits.error_count = 3;
	}

	port_connection_states[port_id] = PortConnectionState::CONFIGURING_ENDPOINTS;

	const configure_endpoint_command_trb cmd{ dev.input_context(), dev.slot_id() };
	xhc.command_ring()->push(cmd);
	xhc.doorbell_register_at(0)->ring(0);
}

void process_event(Controller& xhc)
{
	if (!xhc.primary_event_ring()->has_front()) {
		return;
	}

	auto* event_trb = xhc.primary_event_ring()->front();
	if (auto* trb = trb_dynamic_cast<transfer_event_trb>(event_trb)) {
		on_event(xhc, *trb);
	} else if (auto* trb =
					   trb_dynamic_cast<port_status_change_event_trb>(event_trb)) {
		on_event(xhc, *trb);
	} else if (auto* trb =
					   trb_dynamic_cast<command_completion_event_trb>(event_trb)) {
		on_event(xhc, *trb);
	} else {
		LOG_ERROR("unknown event trb type %d", event_trb->bits.trb_type);
	}

	xhc.primary_event_ring()->pop();
}

Controller* host_controller;

void initialize()
{
	LOG_INFO("Initializing xHCI...");

	kernel::hw::pci::Device* xhc_dev = nullptr;
	for (int i = 0; i < kernel::hw::pci::num_devices; i++) {
		if (kernel::hw::pci::devices[i].is_xhci()) {
			xhc_dev = &kernel::hw::pci::devices[i];

			if (xhc_dev->is_intel()) {
				break;
			}
		}
	}

	if (xhc_dev == nullptr) {
		LOG_ERROR("xHCI device not found.");
		return;
	}

	const uint8_t bsp_lapic_id = *reinterpret_cast<uint32_t*>(0xfee00020) >> 24;
	kernel::hw::pci::configure_msi_fixed_destination(
			*xhc_dev, bsp_lapic_id, kernel::hw::pci::MsiTriggerMode::LEVEL,
			kernel::hw::pci::MsiDeliveryMode::FIXED, kernel::interrupt::InterruptVector::XHCI, 0);

	const uint64_t bar = kernel::hw::pci::read_base_address_register(*xhc_dev, 0);
	const uint64_t xhc_mmio_base = bar & ~static_cast<uint64_t>(0xf);

	host_controller = new Controller(xhc_mmio_base);

	Controller& xhc = *host_controller;

	xhc.initialize();

	xhc.run();

	for (int i = 1; i < xhc.max_ports(); i++) {
		auto p = xhc.port_at(i);

		if (!p.is_connected()) {
			continue;
		}

		configure_port(xhc, p);
	}

	LOG_INFO("xHCI initialized successfully.");
}

void process_events()
{
	while (host_controller->primary_event_ring()->has_front()) {
		process_event(*host_controller);
	}
}
} // namespace kernel::hw::usb::xhci