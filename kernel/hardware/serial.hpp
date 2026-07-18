/**
 * @file hardware/serial.hpp
 * @brief Polled COM1 serial output for headless logging
 */

#pragma once

namespace kernel::hw::serial
{

/**
 * @brief Initialize the COM1 UART for polled output
 *
 * Programs the 16550 UART at COM1 for 115200 baud, 8N1, with FIFOs
 * enabled. Must be called once before write_string() is used.
 */
void initialize();

/**
 * @brief Write a NUL-terminated string to COM1
 *
 * Polls the line status register until the transmit holding register is
 * empty before writing each byte. '\n' is translated to "\r\n" so output
 * displays correctly on a serial terminal.
 *
 * @param s NUL-terminated string to write; ignored if nullptr
 *
 * @note Does nothing if initialize() has not been called yet
 */
void write_string(const char* s);

} // namespace kernel::hw::serial
