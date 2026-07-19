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

namespace kernel::hw::usb::xhci
{
class Controller;
struct PortRegisterSet;
class Device;

// PORTSC (Port Status and Control) is a mixed-access register (xHCI spec
// Table 5-27): some fields are RWS state that must be preserved across a
// write (PLS, PP, PIC, the wake-enable bits), others are RW1CS "*Change"
// bits that must be written as 0 unless the caller wants to clear that
// specific change, and the rest are RO/RsvdZ (safe to write as 0).
//
// The two preserve-masks below keep the same RWS fields but are used for two
// different writes, and are intentionally kept distinct rather than unified:
//   - PORTSC_RESET_PRESERVE_MASK additionally zeroes LWS (bit 16), because a
//     plain port reset must not be interpreted as also requesting a
//     PLS/PIC transition.
//   - PORTSC_CLEAR_CHANGE_BIT_PRESERVE_MASK keeps LWS as last read, since
//     clearing a single "*Change" bit is not a reset.
inline constexpr uint32_t PORTSC_RESET_PRESERVE_MASK = 0x0e00c3e0U;
inline constexpr uint32_t PORTSC_CLEAR_CHANGE_BIT_PRESERVE_MASK = 0x0e01c3e0U;

inline constexpr uint32_t PORTSC_PORT_RESET_BIT = 1U << 4;			   ///< PR
inline constexpr uint32_t PORTSC_CONNECT_STATUS_CHANGE_BIT = 1U << 17; ///< CSC
inline constexpr uint32_t PORTSC_PORT_RESET_CHANGE_BIT = 1U << 21;	   ///< PRC

// Bits ORed in with PORTSC_RESET_PRESERVE_MASK to start a port reset: PR
// requests the reset, CSC clears any stale Connect Status Change picked up
// before the reset was requested.
inline constexpr uint32_t PORTSC_RESET_SET_BITS =
		PORTSC_PORT_RESET_BIT | PORTSC_CONNECT_STATUS_CHANGE_BIT;

class Port
{
public:
	Port(uint8_t port_num, PortRegisterSet& regs) : port_num_(port_num), regs_(regs)
	{
	}

	uint8_t number() const;
	bool is_connected() const;
	bool is_enabled() const;
	bool is_connect_status_changed() const;
	bool is_port_reset_changed() const;
	int speed() const;

	/**
	 * @brief Reset the port
	 * @return true on success, false when the reset does not complete in time
	 */
	bool reset();

	void clear_connect_status_changed()
	{
		clear_status_change_bit(PORTSC_CONNECT_STATUS_CHANGE_BIT);
	}

	void clear_port_reset_changed()
	{
		clear_status_change_bit(PORTSC_PORT_RESET_CHANGE_BIT);
	}

private:
	/**
	 * @brief Clear a single RW1CS "*Change" bit in PORTSC without disturbing
	 * the other preserved fields
	 * @param change_bit The bit to set, e.g. PORTSC_CONNECT_STATUS_CHANGE_BIT
	 */
	void clear_status_change_bit(uint32_t change_bit)
	{
		port_sc_bitmap portsc = regs_.port_sc.read();
		portsc.data[0] &= PORTSC_CLEAR_CHANGE_BIT_PRESERVE_MASK;
		portsc.data[0] |= change_bit;
		regs_.port_sc.write(portsc);
	}

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