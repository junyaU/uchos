/**
 * @file screen.hpp
 * @brief Screen management and framebuffer operations
 */

#pragma once

struct FrameBufferConf;

#include "color.hpp"
#include "point2d.hpp"
#include <cstdint>

namespace kernel::graphics
{

/**
 * @brief Screen class for managing framebuffer and graphics output
 *
 * This class provides an abstraction over the framebuffer, allowing
 * pixel-level drawing operations and managing screen dimensions.
 */
class screen
{
public:
	/**
	 * @brief Construct a new screen object
	 *
	 * @param frame_buffer_conf Framebuffer configuration from UEFI
	 * @param bg_color Background color to use for clearing the screen
	 */
	screen(const FrameBufferConf& frame_buffer_conf, Color bg_color);

	/**
	 * @brief Get screen width in pixels
	 *
	 * @return int Horizontal resolution
	 */
	int width() const { return horizontal_resolution_; }

	/**
	 * @brief Get screen height in pixels
	 *
	 * @return int Vertical resolution
	 */
	int height() const { return vertical_resolution_; }

	/**
	 * @brief Get screen dimensions as a point
	 *
	 * @return point2d Screen width and height
	 */
	point2d size() const { return { width(), height() }; }

	/**
	 * @brief Get the background color
	 *
	 * @return Color Current background color
	 */
	Color bg_color() const { return bg_color_; }

	/**
	 * @brief Draw a single pixel on the screen
	 *
	 * @param point Pixel coordinates
	 * @param color_code Color in 32-bit format
	 *
	 * @note No bounds checking is performed for performance
	 */
	void put_pixel(point2d point, uint32_t color_code);

	/**
	 * @brief Fill a rectangular area with a solid color
	 *
	 * @param position Top-left corner of the rectangle
	 * @param size Width and height of the rectangle
	 * @param color_code Color in 32-bit format
	 */
	void fill_rectangle(point2d position, point2d size, uint32_t color_code);

private:
	uint64_t pixels_per_scan_line_;	 ///< Number of pixels per scan line (may include
									 ///< padding)
	uint64_t horizontal_resolution_; ///< Screen width in pixels
	uint64_t vertical_resolution_;	 ///< Screen height in pixels
	Color bg_color_;				 ///< Background color
	uint32_t* frame_buffer_;		 ///< Pointer to framebuffer memory
};

/**
 * @brief Global kernel screen instance
 *
 * Primary screen object used for all kernel graphics output.
 */
extern screen* kscreen;

/**
 * @brief Initialize the screen subsystem
 *
 * Creates the global screen instance with the provided framebuffer
 * configuration and background color.
 *
 * @param frame_buffer_conf Framebuffer configuration from UEFI
 * @param bg_color Initial background color
 */
void initialize(const FrameBufferConf& frame_buffer_conf, Color bg_color);

} // namespace kernel::graphics
