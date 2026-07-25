/**
 * @file keyboard.hpp
 * @brief Keyboard input handling
 */

#pragma once

namespace kernel::hw
{

/**
 * @brief Initialize the keyboard subsystem
 *
 * Installs the USB keyboard observer. Raw key events (keycode + modifier
 * byte + press/release) are forwarded verbatim to the task that claimed
 * the keyboard focus via INPUT_SET_FOCUS; keymap conversion lives in user
 * space (issue #315).
 */
void initialize_keyboard();

} // namespace kernel::hw
