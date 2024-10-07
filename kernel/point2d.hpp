#pragma once

class point2d
{
public:
	point2d(int x, int y) : x_{ x }, y_{ y } {};
	int GetX() const { return x_; }
	int GetY() const { return y_; }

	point2d operator+(const point2d& rhs) const
	{
		return point2d{ x_ + rhs.x_, y_ + rhs.y_ };
	}

private:
	int x_;
	int y_;
};