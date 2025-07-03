/**
 * @file color.hpp
 * @brief Color representation and manipulation utilities for graphics operations
 */

#pragma once

#include <cstdint>

namespace kernel::graphics
{

/**
 * @brief Number of bytes per pixel in the framebuffer
 *
 * This constant defines the pixel format used by the graphics system.
 * A value of 4 typically indicates 32-bit color depth (RGBA or XRGB).
 */
constexpr int kBytesPerPixel = 4;

/**
 * @brief Represents a color in RGB format
 *
 * This class provides a representation of a 24-bit RGB color where each
 * color component (red, green, blue) is represented by an 8-bit value.
 * Colors can be converted to 32-bit codes for framebuffer operations.
 */
class Color
{
public:
	/**
	 * @brief Construct a new Color object
	 *
	 * @param r Red component value (0-255)
	 * @param g Green component value (0-255)
	 * @param b Blue component value (0-255)
	 */
	constexpr Color(uint8_t r, uint8_t g, uint8_t b) : r_{ r }, g_{ g }, b_{ b } {}

	/**
	 * @brief Get the 32-bit color code for framebuffer operations
	 *
	 * Converts the RGB components into a 32-bit color code suitable for
	 * writing to the framebuffer. The exact format depends on the
	 * framebuffer's pixel format.
	 *
	 * @return uint32_t The 32-bit color code
	 */
	uint32_t GetCode() const;

private:
	uint8_t r_; ///< Red component (0-255)
	uint8_t g_; ///< Green component (0-255)
	uint8_t b_; ///< Blue component (0-255)
};

/**
 * @brief Predefined color constants for common colors
 * @{
 */
constexpr Color WHITE = { 255, 255, 255 };		   ///< Pure white
constexpr Color LIGHT_GRAY = Color(192, 192, 192); ///< Light gray (75% white)
constexpr Color GRAY = Color(128, 128, 128);	   ///< Medium gray (50% white)
constexpr Color DARK_GRAY = Color(64, 64, 64);	   ///< Dark gray (25% white)
constexpr Color BLACK = Color(0, 0, 0);			   ///< Pure black
constexpr Color RED = Color(255, 0, 0);			   ///< Pure red
constexpr Color GREEN = Color(0, 255, 0);		   ///< Pure green
constexpr Color BLUE = Color(0, 0, 255);		   ///< Pure blue
/** @} */

} // namespace kernel::graphics
