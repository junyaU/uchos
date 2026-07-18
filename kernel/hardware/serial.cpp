#include "hardware/serial.hpp"
#include <cstddef>
#include <cstdint>
#include "asm_utils.h"

namespace
{
constexpr uint16_t COM1_BASE = 0x3f8;

constexpr uint16_t REG_DATA = COM1_BASE + 0;		 ///< Data register (DLL when DLAB=1)
constexpr uint16_t REG_INT_ENABLE = COM1_BASE + 1;	 ///< Interrupt enable (DLM when DLAB=1)
constexpr uint16_t REG_FIFO_CTRL = COM1_BASE + 2;	 ///< FIFO control register
constexpr uint16_t REG_LINE_CTRL = COM1_BASE + 3;	 ///< Line control register (DLAB bit)
constexpr uint16_t REG_MODEM_CTRL = COM1_BASE + 4;	 ///< Modem control register
constexpr uint16_t REG_LINE_STATUS = COM1_BASE + 5; ///< Line status register

constexpr uint8_t LCR_DLAB = 0x80;			  ///< Divisor latch access bit
constexpr uint8_t LCR_8N1 = 0x03;			  ///< 8 data bits, no parity, 1 stop bit
constexpr uint8_t FCR_ENABLE_CLEAR_14 = 0xc7; ///< Enable/clear FIFOs, 14-byte threshold
constexpr uint8_t MCR_DTR_RTS_OUT2 = 0x0b;	  ///< DTR | RTS | OUT2
constexpr uint8_t LSR_THR_EMPTY = 0x20;	  ///< Transmit holding register empty

bool initialized = false;
} // namespace

namespace kernel::hw::serial
{

void initialize()
{
	write_to_io_port8(REG_INT_ENABLE, 0x00); // disable all interrupts
	write_to_io_port8(REG_LINE_CTRL, LCR_DLAB);
	write_to_io_port8(REG_DATA, 0x01);		 // divisor low byte: 115200 baud
	write_to_io_port8(REG_INT_ENABLE, 0x00); // divisor high byte
	write_to_io_port8(REG_LINE_CTRL, LCR_8N1);
	write_to_io_port8(REG_FIFO_CTRL, FCR_ENABLE_CLEAR_14);
	write_to_io_port8(REG_MODEM_CTRL, MCR_DTR_RTS_OUT2);

	initialized = true;
}

// Serial mirror of the kernel log so headless CI can observe test results.
void write_string(const char* s)
{
	if (!initialized || s == nullptr) {
		return;
	}

	for (size_t i = 0; s[i] != '\0'; ++i) {
		if (s[i] == '\n') {
			while ((read_from_io_port8(REG_LINE_STATUS) & LSR_THR_EMPTY) == 0) {
			}
			write_to_io_port8(REG_DATA, '\r');
		}

		while ((read_from_io_port8(REG_LINE_STATUS) & LSR_THR_EMPTY) == 0) {
		}
		write_to_io_port8(REG_DATA, static_cast<uint8_t>(s[i]));
	}
}

} // namespace kernel::hw::serial
