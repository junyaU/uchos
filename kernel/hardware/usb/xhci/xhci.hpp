/**
 * @file hardware/usb/xhci/xhci.hpp
 *
 * @brief xHCI controller driver
 *
 * Provides an interface to manage and interact with the xHCI USB host controller.
 * This includes initializing and running the controller, handling commands and
 * events, and managing connected USB devices and ports.
 *
 */

#pragma once

#include "device_manager.hpp"
#include "port.hpp"
#include "registers.hpp"
#include "ring.hpp"
#include <cstdint>
#include <cstring>

namespace usb::xhci
{
class controller
{
public:
	controller(uintptr_t mmio_base);
	void initialize();
	void run();
	ring* command_ring() { return &command_ring_; }
	event_ring* primary_event_ring() { return &event_ring_; }
	doorbell_register* doorbell_register_at(uint8_t index);
	port port_at(uint8_t index)
	{
		return port{ index, port_register_sets()[index - 1] };
	}
	uint8_t max_ports() const { return max_ports_; }
	device_manager* device_manager() { return &device_manager_; }

private:
	static const size_t DEVICE_SIZE = 8;

	const uintptr_t mmio_base_;
	capability_registers* const cap_regs_;
	operational_registers* const op_regs_;
	const uint8_t max_ports_;

	class device_manager device_manager_;
	ring command_ring_;
	event_ring event_ring_;

	interrupter_register_set_array interrupter_register_sets() const
	{
		return { mmio_base_ + cap_regs_->rts_offset.read().offset() + 0x20U, 1024 };
	}

	port_register_set_array port_register_sets() const
	{
		return { reinterpret_cast<uintptr_t>(op_regs_) + 0x400U, max_ports_ };
	}

	doorbell_register_array doorbell_registers() const
	{
		return { mmio_base_ + cap_regs_->doorbell_offset.read().offset(), 256 };
	}
};

void configure_port(controller& xhc, port& p);
void configure_endpoints(controller& xhc, device& dev);

void process_event(controller& xhc);

extern controller host_controller;
void initialize();
void process_events();
} // namespace usb::xhci