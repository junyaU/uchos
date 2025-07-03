/**
 * @file point2d.hpp
 * @brief 2D point class for coordinate representation
 * 
 * This file provides a simple 2D point class used for representing
 * coordinates in the graphics subsystem. It supports basic operations
 * like addition for coordinate transformations.
 * 
 * @date 2024
 */

#pragma once

/**
 * @brief A 2D point with integer coordinates
 * 
 * This class represents a point in 2D space using integer coordinates.
 * It is primarily used in the graphics subsystem for screen positioning,
 * cursor tracking, and drawing operations.
 */
class point2d
{
public:
	/**
	 * @brief Construct a new 2D point
	 * 
	 * @param x The x-coordinate
	 * @param y The y-coordinate
	 */
	point2d(int x, int y) : x_{ x }, y_{ y } {};
	
	/**
	 * @brief Get the x-coordinate
	 * 
	 * @return The x-coordinate value
	 */
	int GetX() const { return x_; }
	
	/**
	 * @brief Get the y-coordinate
	 * 
	 * @return The y-coordinate value
	 */
	int GetY() const { return y_; }

	/**
	 * @brief Add two points together
	 * 
	 * Performs component-wise addition of two points, useful for
	 * translating positions or calculating offsets.
	 * 
	 * @param rhs The point to add to this point
	 * @return A new point representing the sum
	 * 
	 * @example
	 * point2d p1(10, 20);
	 * point2d p2(5, 7);
	 * point2d result = p1 + p2;  // result is (15, 27)
	 */
	point2d operator+(const point2d& rhs) const
	{
		return point2d{ x_ + rhs.x_, y_ + rhs.y_ };
	}

private:
	int x_;  ///< The x-coordinate
	int y_;  ///< The y-coordinate
};