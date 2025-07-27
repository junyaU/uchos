#include "port.hpp"
#include <cstdint>
#include "graphics/log.hpp"
#include "registers.hpp"

namespace kernel::hw::usb::xhci
{
uint8_t Port::number() const { return port_num_; }

bool Port::is_connected() const
{
	return regs_.port_sc.read().bits.current_connect_status;
}

bool Port::is_enabled() const
{
	return regs_.port_sc.read().bits.port_enabled_disabled;
}

bool Port::is_connect_status_changed() const
{
	return regs_.port_sc.read().bits.connect_status_change;
}

bool Port::is_port_reset_changed() const
{
	return regs_.port_sc.read().bits.port_reset_change;
}

int Port::speed() const { return regs_.port_sc.read().bits.port_speed; }

void Port::reset()
{
	auto portsc = regs_.port_sc.read();
	portsc.data[0] &= 0x0e00c3e0U;
	portsc.data[0] |= 0x00020010U;
	regs_.port_sc.write(portsc);
	while (regs_.port_sc.read().bits.port_reset) {
	}
}

void reset_port(Port& p)
{
	const bool is_connected = p.is_connected();
	if (!is_connected) {
		return;
	}

	if (addressing_port != 0) {
		port_connection_states[p.number()] =
				PortConnectionState::WAITING_ADDRESSED;
		return;
	}

	const auto port_state = port_connection_states[p.number()];
	if (port_state != PortConnectionState::DISCONNECTED &&
		port_state != PortConnectionState::WAITING_ADDRESSED) {
		LOG_ERROR("port %d is not disconnected or waiting addressed", p.number());
		return;
	}

	addressing_port = p.number();
	port_connection_states[p.number()] = PortConnectionState::RESETTING_PORT;
	p.reset();
}

void configure_port(Port& p)
{
	if (port_connection_states[p.number()] == PortConnectionState::DISCONNECTED) {
		reset_port(p);
	}
}

} // namespace kernel::hw::usb::xhci