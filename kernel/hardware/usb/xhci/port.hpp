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

#include <array>
#include <cstdint>

#define CLEAR_STATUS_BIT(bitname)                                                   \
	[this]() {                                                                      \
		port_sc_bitmap portsc = regs_.port_sc.read();                               \
		portsc.data[0] &= 0x0e01c3e0u;                                              \
		portsc.bits.bitname = 1;                                                    \
		regs_.port_sc.write(portsc);                                                \
	}()

namespace kernel::hw::usb::xhci
{
class Controller;
struct PortRegisterSet;
class Device;

class Port
{
public:
	Port(uint8_t port_num, PortRegisterSet& regs)
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

	void clear_connect_status_changed() { CLEAR_STATUS_BIT(connect_status_change); }

	void clear_port_reset_changed() { CLEAR_STATUS_BIT(port_reset_change); }

private:
	const uint8_t port_num_;
	PortRegisterSet& regs_;
};

enum class PortConnectionState {
	DISCONNECTED,
	WAITING_ADDRESSED,
	RESETTING_PORT,
	ENABLEING_SLOT,
	ADDRESSING_DEVICE,
	INITIALIZING_DEVICE,
	CONFIGURING_ENDPOINTS,
	CONFIGURED,
};

inline std::array<volatile PortConnectionState, 256> port_connection_states{};

inline uint8_t addressing_port{ 0 };

void reset_port(Port& p);

void configure_port(Port& p);
} // namespace kernel::hw::usb::xhci