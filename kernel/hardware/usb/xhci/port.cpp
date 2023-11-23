#include "port.hpp"
#include "registers.hpp"
#include "xhci.hpp"

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

device* port::initialize() { return nullptr; }
} // namespace usb::xhci