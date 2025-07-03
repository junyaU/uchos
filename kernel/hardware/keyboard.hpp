/**
 * @file keyboard.hpp
 * @brief Keyboard input handling and modifier key definitions
 */

#pragma once

#include <cstdint>

namespace kernel::hw
{

/**
 * @brief Keyboard modifier key bit masks
 *
 * These constants define the bit positions for modifier keys in the
 * keyboard modifier byte. Each bit represents the state of a specific
 * modifier key.
 * @{
 */
static const int L_CONTROL = 0b00000001U; ///< Left Control key
static const int L_SHIFT = 0b00000010U;	  ///< Left Shift key
static const int L_ALT = 0b00000100U;	  ///< Left Alt key
static const int L_GUI = 0b00001000U;	  ///< Left GUI/Windows/Command key
static const int R_CONTROL = 0b00010000U; ///< Right Control key
static const int R_SHIFT = 0b00100000U;	  ///< Right Shift key
static const int R_ALT = 0b01000000U;	  ///< Right Alt key
static const int R_GUI = 0b10000000U;	  ///< Right GUI/Windows/Command key
/** @} */

/**
 * @brief Initialize the keyboard subsystem
 *
 * Sets up keyboard input handling, including interrupt handlers,
 * keymaps, and any necessary hardware configuration.
 */
void initialize_keyboard();

/**
 * @brief Check if the key combination represents End-Of-Transmission (EOT)
 *
 * Tests whether the given modifier and keycode combination represents
 * the EOT control sequence (typically Ctrl+D in Unix systems).
 *
 * @param modifier Keyboard modifier byte containing modifier key states
 * @param keycode The keycode of the pressed key
 * @return true if the combination represents EOT
 * @return false otherwise
 *
 * @note EOT is commonly used to signal end of input in terminals
 */
bool is_EOT(uint8_t modifier, uint8_t keycode);

} // namespace kernel::hw
