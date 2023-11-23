/**
 * @file hardware/usb/xhci/port.hpp
 *
 * @brief Port class for xHCI
 *
 * This class represents a single USB port managed by the xHCI (eXtensible Host
 * Controller Interface). It provides methods to interact with the port, such as
 * checking its status, resetting it, and initializing connected devices.
 *
 */

#pragma once

#include "registers.hpp"

#include <cstdint>

#define CLEAR_STATUS_BIT(bitname)                                                   \
	[this]() {                                                                      \
		port_sc_bitmap portsc = regs_.port_sc.read();                               \
		portsc.data[0] &= 0x0e01c3e0u;                                              \
		portsc.bits.bitname = 1;                                                    \
		regs_.port_sc.write(portsc);                                                \
	}()

namespace usb::xhci
{
class controller;
struct port_register_set;
class device;

class port
{
public:
	port(uint8_t port_num, port_register_set& regs)
		: port_num_(port_num), regs_(regs)
	{
	}

	uint8_t number() const;
	bool is_connected() const;
	bool is_enabled() const;
	bool is_connect_status_changed() const;
	bool is_port_reset_changed() const;
	int speed() const;
	void reset();
	device* initialize();

	void clear_connect_status_changed() { CLEAR_STATUS_BIT(connect_status_change); }

	void clear_port_reset_changed() { CLEAR_STATUS_BIT(port_reset_change); }

private:
	const uint8_t port_num_;
	port_register_set& regs_;
};
} // namespace usb::xhci