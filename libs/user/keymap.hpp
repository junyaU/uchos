/**
 * @file keymap.hpp
 * @brief US-layout keymap: raw USB HID keycode to ASCII
 *
 * The kernel delivers raw key events (issue #315); turning them into
 * characters is user-space policy. This is the default US layout used by
 * the shell; a future input server can swap it per layout.
 */

#pragma once

#include <cstdint>

/**
 * @brief Convert a raw USB HID keycode + modifier byte to ASCII (US layout)
 * @param keycode USB HID usage id from NOTIFY_KEY_INPUT
 * @param modifier KEY_MOD_* bits; only shift affects the mapping
 * @return The ASCII character, or 0 when the key has no printable mapping
 */
char keycode_to_ascii(uint8_t keycode, uint8_t modifier);
