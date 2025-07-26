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
class Point2D
{
public:
	/**
	 * @brief Construct a new 2D point
	 * 
	 * @param x The x-coordinate
	 * @param y The y-coordinate
	 */
	Point2D(int x, int y) : x_{ x }, y_{ y } {};
	
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
	 * Point2D p1(10, 20);
	 * Point2D p2(5, 7);
	 * Point2D result = p1 + p2;  // result is (15, 27)
	 */
	Point2D operator+(const Point2D& rhs) const
	{
		return Point2D{ x_ + rhs.x_, y_ + rhs.y_ };
	}

private:
	int x_;  ///< The x-coordinate
	int y_;  ///< The y-coordinate
};