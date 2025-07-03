/**
 * @file font.hpp
 * @brief Font rendering and text display utilities
 */

#pragma once

#include <cstdint>

#include <ft2build.h>
#include FT_FREETYPE_H

#include "../point2d.hpp"

namespace kernel::graphics
{

/**
 * @brief Bitmap font class for fixed-width font rendering
 *
 * This class manages bitmap fonts where each character is represented
 * as a fixed-size bitmap. Used for basic text rendering in the kernel.
 */
class bitmap_font
{
public:
	/**
	 * @brief Construct a new bitmap font
	 *
	 * @param width Width of each character in pixels
	 * @param height Height of each character in pixels
	 */
	bitmap_font(int width, int height);

	/**
	 * @brief Get the bitmap data for a character
	 *
	 * @param c ASCII character to retrieve
	 * @return const uint8_t* Pointer to bitmap data for the character
	 */
	const uint8_t* get_font(char c);

	/**
	 * @brief Get the character width
	 *
	 * @return int Width in pixels
	 */
	int width() const { return width_; }

	/**
	 * @brief Get the character height
	 *
	 * @return int Height in pixels
	 */
	int height() const { return height_; }

	/**
	 * @brief Get the character size as a point
	 *
	 * @return point2d Character dimensions
	 */
	point2d size() const { return { width_, height_ }; }

private:
	const uint8_t* font_data_; ///< Pointer to font bitmap data
	int width_;				   ///< Character width in pixels
	int height_;			   ///< Character height in pixels
};

class screen;

/**
 * @brief Check if a character is within ASCII range
 *
 * @param c Unicode character to check
 * @return true if character is ASCII (0-127)
 * @return false otherwise
 */
bool is_ascii_code(char32_t c);

/**
 * @brief Get the number of bytes in a UTF-8 sequence
 *
 * Determines the length of a UTF-8 character sequence based on
 * the first byte.
 *
 * @param c First byte of UTF-8 sequence
 * @return int Number of bytes (1-4) or 0 if invalid
 */
int utf8_size(uint8_t c);

/**
 * @brief Convert UTF-8 string to Unicode code point
 *
 * @param utf8 Pointer to UTF-8 encoded string
 * @return char32_t Unicode code point
 */
char32_t utf8_to_unicode(const char* utf8);

/**
 * @brief Decode Unicode code point to displayable character
 *
 * Maps Unicode code points to characters that can be displayed
 * with the available font.
 *
 * @param c Unicode code point
 * @return char Displayable character or replacement character
 */
char decode_utf8(char32_t c);

/**
 * @brief Write a single ASCII character to the screen
 *
 * @param scr Screen to write to
 * @param position Character position on screen
 * @param c ASCII character to write
 * @param color_code Color in 32-bit format
 */
void write_ascii(screen& scr, point2d position, char c, uint32_t color_code);

/**
 * @brief Write a Unicode character to the screen
 *
 * @param scr Screen to write to
 * @param position Character position on screen
 * @param c Unicode character to write
 * @param color_code Color in 32-bit format
 *
 * @note Falls back to ASCII or replacement character if Unicode
 *       character cannot be displayed
 */
void write_unicode(screen& scr, point2d position, char32_t c, uint32_t color_code);

/**
 * @brief Write a string to the screen
 *
 * Supports UTF-8 encoded strings and handles multi-byte characters.
 *
 * @param scr Screen to write to
 * @param position Starting position for the string
 * @param s Null-terminated string (UTF-8 encoded)
 * @param color_code Color in 32-bit format
 */
void write_string(screen& scr, point2d position, const char* s, uint32_t color_code);

/**
 * @brief Convert string to lowercase
 *
 * @param s String to convert (modified in place)
 *
 * @note Only converts ASCII characters
 */
void to_lower(char* s);

/**
 * @brief Convert string to uppercase
 *
 * @param s String to convert (modified in place)
 *
 * @note Only converts ASCII characters
 */
void to_upper(char* s);

/**
 * @brief Global kernel font instance
 *
 * The primary font used for kernel text output.
 */
extern bitmap_font* kfont;

/**
 * @brief Create a new FreeType font face
 *
 * Loads a font face from the embedded font data using FreeType.
 *
 * @return FT_Face Newly created font face
 */
FT_Face new_face();

/**
 * @brief Initialize the font subsystem
 *
 * Sets up the bitmap font for kernel text rendering.
 */
void initialize_font();

/**
 * @brief Initialize the FreeType library
 *
 * Initializes FreeType for advanced font rendering capabilities.
 * Must be called before using any FreeType functions.
 */
void initialize_freetype();

} // namespace kernel::graphics