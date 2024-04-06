#include "port.hpp"
#include "../../../graphics/log.hpp"
#include "registers.hpp"
#include <libs/common/types.hpp>

namespace usb::xhci
{
uint8_t port::number() const { return port_num_; }

bool port::is_connected() const
{
	return regs_.port_sc.read().bits.current_connect_status;
}

bool port::is_enabled() const
{
	return regs_.port_sc.read().bits.port_enabled_disabled;
}

bool port::is_connect_status_changed() const
{
	return regs_.port_sc.read().bits.connect_status_change;
}

bool port::is_port_reset_changed() const
{
	return regs_.port_sc.read().bits.port_reset_change;
}

int port::speed() const { return regs_.port_sc.read().bits.port_speed; }

void port::reset()
{
	auto portsc = regs_.port_sc.read();
	portsc.data[0] &= 0x0e00c3e0U;
	portsc.data[0] |= 0x00020010U;
	regs_.port_sc.write(portsc);
	while (regs_.port_sc.read().bits.port_reset) {
	}
}

void reset_port(port& p)
{
	const bool is_connected = p.is_connected();
	if (!is_connected) {
		return;
	}

	if (addressing_port != 0) {
		port_connection_states[p.number()] =
				port_connection_state::WAITING_ADDRESSED;
		return;
	}

	const auto port_state = port_connection_states[p.number()];
	if (port_state != port_connection_state::DISCONNECTED &&
		port_state != port_connection_state::WAITING_ADDRESSED) {
		printk(KERN_ERROR, "port %d is not disconnected or waiting addressed",
			   p.number());
		return;
	}

	addressing_port = p.number();
	port_connection_states[p.number()] = port_connection_state::RESETTING_PORT;
	p.reset();
}

void configure_port(port& p)
{
	if (port_connection_states[p.number()] == port_connection_state::DISCONNECTED) {
		reset_port(p);
	}
}

} // namespace usb::xhci