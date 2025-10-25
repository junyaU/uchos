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

#include <cstdint>
#include <cstring>
#include "device_manager.hpp"
#include "port.hpp"
#include "registers.hpp"
#include "ring.hpp"

namespace kernel::hw::usb::xhci
{
class Controller
{
public:
	Controller(uintptr_t mmio_base);
	void initialize();
	void run();
	Ring* command_ring() { return &command_ring_; }
	EventRing* primary_event_ring() { return &event_ring_; }
	DoorbellRegister* doorbell_register_at(uint8_t index);
	Port port_at(uint8_t index)
	{
		return Port{ index, port_register_sets()[index - 1] };
	}
	uint8_t max_ports() const { return max_ports_; }
	DeviceManager* device_manager() { return &device_manager_; }

private:
	static const size_t DEVICE_SIZE = 8;

	const uintptr_t mmio_base_;
	CapabilityRegisters* const cap_regs_;
	OperationalRegisters* const op_regs_;
	const uint8_t max_ports_;

	class DeviceManager device_manager_;
	Ring command_ring_;
	EventRing event_ring_;

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

void configure_port(Controller& xhc, Port& p);
void configure_endpoints(Controller& xhc, Device& dev);

void process_event(Controller& xhc);

extern Controller* host_controller;
void initialize();
void process_events();
} // namespace kernel::hw::usb::xhci