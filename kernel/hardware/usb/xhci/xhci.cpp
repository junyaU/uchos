#include "xhci.hpp"
#include "../../../asm_utils.h"
#include "../../../graphics/kernel_logger.hpp"
#include "../../../interrupt/vector.hpp"
#include "../../pci.hpp"
#include "../memory.hpp"
#include "context.hpp"
#include "speed.hpp"
#include "trb.hpp"

namespace
{
using namespace usb::xhci;

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

void enable_slot(controller& xhc, port& p)
{
	const bool is_enabled = p.is_enabled();
	const bool reset_completed = p.is_port_reset_changed();

	if (is_enabled && reset_completed) {
		p.clear_port_reset_changed();
		port_connection_states[p.number()] = port_connection_state::ENABLEING_SLOT;

		enable_slot_command_trb cmd{};
		xhc.command_ring()->push(cmd);
		xhc.doorbell_register_at(0)->ring(0);
	}
}

void initialize_device(controller& xhc, uint8_t port_id, uint8_t slot_id)
{
	auto* dev = xhc.device_manager()->find_by_slot_id(slot_id);
	if (dev == nullptr) {
		klogger->printf("device not found for slot id %d\n", slot_id);
		return;
	}

	port_connection_states[port_id] = port_connection_state::INITIALIZING_DEVICE;
	dev->start_initialize();
}

void complete_configuration(controller& xhc, uint8_t port_id, uint8_t slot_id)
{
	auto dev = xhc.device_manager()->find_by_slot_id(slot_id);
	if (dev == nullptr) {
		klogger->printf("device not found for slot id %d\n", slot_id);
		return;
	}

	dev->on_endpoints_configured();

	port_connection_states[port_id] = port_connection_state::CONFIGURED;
}

void address_device(controller& xhc, uint8_t port_id, uint8_t slot_id)
{
	xhc.device_manager()->allocate_device(slot_id,
										  xhc.doorbell_register_at(slot_id));

	device* dev = xhc.device_manager()->find_by_slot_id(slot_id);
	if (dev == nullptr) {
		klogger->printf("device not found for slot id %d\n", slot_id);
		return;
	}

	memset(&dev->input_context()->control, 0, sizeof(input_control_context));

	const auto ep0_dci = device_context_index{ 0, false };
	auto* slot_ctx = dev->input_context()->enable_slot_context();
	auto* ep0_ctx = dev->input_context()->enable_endpoint_context(ep0_dci);

	auto port = xhc.port_at(port_id);
	initialize_slot_context(*slot_ctx, port_id, port.speed());

	initialize_endpoint0_context(
			*ep0_ctx, dev->alloc_transfer_ring(ep0_dci, 32)->buffer(),
			determine_max_packet_size_for_control_pipe(slot_ctx->bits.speed));

	xhc.device_manager()->load_dcbaa(slot_id);

	port_connection_states[port_id] = port_connection_state::ADDRESSING_DEVICE;

	address_device_command_trb cmd{ dev->input_context(), slot_id };
	xhc.command_ring()->push(cmd);
	xhc.doorbell_register_at(0)->ring(0);
}

void on_event(controller& xhc, port_status_change_event_trb& trb)
{
	auto port_id = trb.bits.port_id;
	auto p = xhc.port_at(port_id);

	switch (port_connection_states[port_id]) {
		case port_connection_state ::DISCONNECTED:
			return reset_port(p);
		case port_connection_state::RESETTING_PORT:
			return enable_slot(xhc, p);
		default:
			klogger->printf("port %d is not disconnected or resetting\n", port_id);
			break;
	}
}

void on_event(controller& xhc, transfer_event_trb& trb)
{
	const uint8_t slot_id = trb.bits.slot_id;
	auto* dev = xhc.device_manager()->find_by_slot_id(slot_id);
	if (dev == nullptr) {
		klogger->printf("device not found for slot id %d\n", slot_id);
		return;
	}

	dev->on_transfer_event_received(trb);

	const auto port_id = dev->context()->slot.bits.root_hub_port_num;
	if (dev->is_initialized() &&
		port_connection_states[port_id] ==
				port_connection_state::INITIALIZING_DEVICE) {
		return configure_endpoints(xhc, *dev);
	}
}

void on_event(controller& xhc, command_completion_event_trb& trb)
{
	const auto issuer_type = trb.pointer()->bits.trb_type;
	const auto slot_id = trb.bits.slot_id;
	if (issuer_type == enable_slot_command_trb::TYPE) {
		if (port_connection_states[addressing_port] !=
			port_connection_state::ENABLEING_SLOT) {
			klogger->printf("port %d is not enableing slot\n", addressing_port);
			return;
		}

		return address_device(xhc, addressing_port, slot_id);
	} else if (issuer_type == address_device_command_trb::TYPE) {
		auto dev = xhc.device_manager()->find_by_slot_id(slot_id);
		if (dev == nullptr) {
			klogger->printf("device not found for slot id %d\n", slot_id);
			return;
		};

		auto port_id = dev->context()->slot.bits.root_hub_port_num;
		if (port_id != addressing_port) {
			klogger->printf("port id %d is not equal to addressing port %d\n",
							port_id, addressing_port);
			return;
		}

		if (port_connection_states[port_id] !=
			port_connection_state::ADDRESSING_DEVICE) {
			klogger->printf("port %d is not addressing device\n", port_id);
			return;
		}

		addressing_port = 0;
		for (int i = 0; i < port_connection_states.size(); i++) {
			if (port_connection_states[i] ==
				port_connection_state::WAITING_ADDRESSED) {
				auto p = xhc.port_at(i);
				reset_port(p);
				break;
			}
		}

		return initialize_device(xhc, port_id, slot_id);
	} else if (issuer_type == configure_endpoint_command_trb::TYPE) {
		auto dev = xhc.device_manager()->find_by_slot_id(slot_id);
		if (dev == nullptr) {
			klogger->printf("device not found for slot id %d\n", slot_id);
			return;
		}

		auto port_id = dev->context()->slot.bits.root_hub_port_num;
		if (port_connection_states[port_id] !=
			port_connection_state::CONFIGURING_ENDPOINTS) {
			klogger->printf("port %d is not configuring endpoints\n", port_id);
			return;
		}

		return complete_configuration(xhc, port_id, slot_id);
	}
}

void request_hc_ownership(uintptr_t mmio_base, hcc_params1_register hccp)
{
	extended_register_list extended_regs{ mmio_base, hccp };

	auto ext_usb_legacy_support =
			std::find_if(extended_regs.begin(), extended_regs.end(), [](auto& reg) {
				return reg.read().bits.capability_id == 1;
			});
	if (ext_usb_legacy_support == extended_regs.end()) {
		return;
	}

	auto& reg = reinterpret_cast<memory_mapped_register<usb_legacy_support_bitmap>&>(
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

namespace usb::xhci
{
controller::controller(uintptr_t mmio_base)
	: mmio_base_(mmio_base),
	  cap_regs_(reinterpret_cast<capability_registers*>(mmio_base)),
	  op_regs_(reinterpret_cast<operational_registers*>(
			  mmio_base + cap_regs_->cap_length.read())),
	  max_ports_(static_cast<uint8_t>(cap_regs_->hcs_params1.read().bits.max_ports))
{
}

void controller::initialize()
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
		auto* scratchpad_buf_arr =
				alloc_array<void*>(max_scratchpad_buffers, 64, 4096);
		for (int i = 0; i < max_scratchpad_buffers; i++) {
			scratchpad_buf_arr[i] = alloc_memory(4096, 4096, 4096);
		}

		device_manager_.device_contexts()[0] =
				reinterpret_cast<device_context*>(scratchpad_buf_arr);
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

void controller::run()
{
	auto usb_command = op_regs_->usb_cmd.read();
	usb_command.bits.run_stop = true;
	op_regs_->usb_cmd.write(usb_command);
	op_regs_->usb_cmd.read();

	while (op_regs_->usb_sts.read().bits.host_controller_halted) {
	}
}

doorbell_register* controller::doorbell_register_at(uint8_t index)
{
	return &doorbell_registers()[index];
}

void configure_port(controller& xhc, port& p)
{
	if (port_connection_states[p.number()] == port_connection_state::DISCONNECTED) {
		reset_port(p);
	}
}

void configure_endpoints(controller& xhc, device& dev)
{
	const auto* configs = dev.endpoint_configs();
	const auto len = dev.num_endpoint_configs();

	memset(&dev.input_context()->control, 0, sizeof(input_control_context));
	memcpy(&dev.input_context()->slot, &dev.context()->slot, sizeof(slot_context));

	auto* slot_ctx = dev.input_context()->enable_slot_context();
	// setting endpoint num
	slot_ctx->bits.context_entries = 31;
	const auto port_id{ dev.context()->slot.bits.root_hub_port_num };
	const int port_speed = xhc.port_at(port_id).speed();
	if (port_speed == 0 || port_speed > SUPER_SPEED_PLUS) {
		klogger->error("unsupported port speed");
		return;
	}

	auto convert_interval = [port_speed](endpoint_type type, int interval) {
		if (port_speed == FULL_SPEED || port_speed == LOW_SPEED) {
			if (type == endpoint_type::ISOCHRONOUS) {
				return interval + 2;
			}
			return most_significant_bit(interval) + 3;
		}

		return interval - 1;
	};

	for (int i = 0; i < len; i++) {
		const device_context_index ep_dci{ configs[i].id };
		auto* ep_ctx = dev.input_context()->enable_endpoint_context(ep_dci);
		switch (configs[i].type) {
			case endpoint_type::CONTROL:
				ep_ctx->bits.ep_type = 4;
				break;
			case endpoint_type::ISOCHRONOUS:
				ep_ctx->bits.ep_type = configs[i].id.is_in() ? 5 : 1;
				break;
			case endpoint_type::BULK:
				ep_ctx->bits.ep_type = configs[i].id.is_in() ? 6 : 2;
				break;
			case endpoint_type::INTERRUPT:
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

	port_connection_states[port_id] = port_connection_state::CONFIGURING_ENDPOINTS;

	configure_endpoint_command_trb cmd{ dev.input_context(), dev.slot_id() };
	xhc.command_ring()->push(cmd);
	xhc.doorbell_register_at(0)->ring(0);
}

void process_event(controller& xhc)
{
	if (!xhc.primary_event_ring()->has_front()) {
		return;
	}

	auto event_trb = xhc.primary_event_ring()->front();
	if (auto trb = trb_dynamic_cast<transfer_event_trb>(event_trb)) {
		on_event(xhc, *trb);
	} else if (auto trb =
					   trb_dynamic_cast<port_status_change_event_trb>(event_trb)) {
		on_event(xhc, *trb);
	} else if (auto trb =
					   trb_dynamic_cast<command_completion_event_trb>(event_trb)) {
		on_event(xhc, *trb);
	} else {
		klogger->printf("unknown event trb type %d\n", event_trb->bits.trb_type);
	}

	xhc.primary_event_ring()->pop();
}

controller* host_controller;

void initialize()
{
	klogger->info("Initializing xHCI...");

	pci::device* xhc_dev = nullptr;
	for (int i = 0; i < pci::num_devices; i++) {
		if (pci::devices[i].is_xhci()) {
			xhc_dev = &pci::devices[i];

			if (xhc_dev->is_intel()) {
				break;
			}
		}
	}

	if (xhc_dev == nullptr) {
		klogger->error("xHCI device not found.");
		return;
	}

	const uint8_t bsp_lapic_id = *reinterpret_cast<uint32_t*>(0xfee00020) >> 24;
	pci::configure_msi_fixed_destination(
			*xhc_dev, bsp_lapic_id, pci::msi_trigger_mode::LEVEL,
			pci::msi_delivery_mode::FIXED, InterruptVector::kXHCI, 0);

	const uint64_t bar = pci::read_base_address_register(*xhc_dev, 0);
	const uint64_t xhc_mmio_base = bar & ~static_cast<uint64_t>(0xf);

	host_controller = new controller(xhc_mmio_base);

	controller& xhc = *host_controller;

	xhc.initialize();

	xhc.run();

	klogger->info("xHCI initialized successfully.");

	for (int i = 1; i < xhc.max_ports(); i++) {
		auto p = xhc.port_at(i);

		if (!p.is_connected()) {
			continue;
		}

		configure_port(xhc, p);
	}
}

void process_events()
{
	while (host_controller->primary_event_ring()->has_front()) {
		process_event(*host_controller);
	}
}
} // namespace usb::xhci