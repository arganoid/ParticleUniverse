#pragma once
#pragma once

#include <cmath>


class Vector2Dbl
{
public:

	static Vector2Dbl zero;
	static Vector2Dbl up;

	double x, y;

	Vector2Dbl() :x(0.0f), y(0.0f)
	{
	}

	Vector2Dbl(double _x, double _y) :x(_x), y(_y)
	{
	}

	__forceinline double GetX() const { return x; }
	__forceinline double GetY() const { return y; }

	__forceinline void SetX(double _x) { x = _x; }
	__forceinline void SetY(double _y) { y = _y; }

	__forceinline void AddToX(double _x) { x += _x; }
	__forceinline void AddToY(double _y) { y += _y; }

	__forceinline double Mag() const { return sqrt(x*x + y*y); }
	__forceinline double MagSq() const { return x*x + y*y; }

	__forceinline Vector2Dbl Normalised() { return *this / Mag(); }

	__forceinline void Normalise() { *this /= Mag(); }
	__forceinline void SetLength(double _newlength) { *this *= (_newlength / Mag()); }

	__forceinline void operator += (Vector2Dbl const& _param)
	{
		x += _param.GetX();
		y += _param.GetY();
	}

	__forceinline void operator = (Vector2Dbl _param)
	{
		x = _param.GetX();
		y = _param.GetY();
	}

	__forceinline Vector2Dbl operator + (Vector2Dbl const& _other) const
	{
		return Vector2Dbl(x + _other.GetX(), y + _other.GetY());
	}

	__forceinline Vector2Dbl operator - (Vector2Dbl const& _other) const
	{
		return Vector2Dbl(x - _other.GetX(), y - _other.GetY());
	}

	__forceinline Vector2Dbl operator - () const
	{
		return Vector2Dbl(-x, -y);
	}

	__forceinline Vector2Dbl operator * (double _mult) const
	{
		return Vector2Dbl(x * _mult, y * _mult);
	}

	__forceinline Vector2Dbl operator / (double _div) const
	{
		return Vector2Dbl(x / _div, y / _div);
	}

	__forceinline void operator *= (double _scale)
	{
		x *= _scale;
		y *= _scale;
	}

	__forceinline void operator /= (double _scale)
	{
		_scale = 1 / _scale;
		x *= _scale;
		y *= _scale;
	}

	// Dot product
	__forceinline double operator * (Vector2Dbl const& _mult) const
	{
		return (x * _mult.GetX()) + (y * _mult.GetY());
	}

	// Rotate anticlockwise
	Vector2Dbl const& Rotate(double _angle)
	{
		double s = (double)sin(_angle);
		double c = (double)cos(_angle);

		double t = c*x + s*y;
		y = -s*x + c*y;
		x = t;

		return *this;
	}

	// Rotate anticlockwise
	Vector2Dbl Rotated(double _angle)
	{
		Vector2Dbl v = *this;

		double s = (double)sin(_angle);
		double c = (double)cos(_angle);

		double t = c*v.x + s*v.y;
		v.y = -s*x + c*v.y;
		v.x = t;

		return v;
	}
};
