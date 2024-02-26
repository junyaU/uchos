#pragma once

class Point2D
{
public:
	Point2D(int x, int y) : x_{ x }, y_{ y } {};
	int GetX() const { return x_; }
	int GetY() const { return y_; }

	Point2D operator+(const Point2D& rhs) const
	{
		return Point2D{ x_ + rhs.x_, y_ + rhs.y_ };
	}

private:
	int x_;
	int y_;
};