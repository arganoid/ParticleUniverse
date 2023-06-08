#pragma once

#include <cmath>
#include <utility>
#include <sstream>

template<typename T>
class Vector2Base
{
public:

	//	inline static Vector2Base<T> zero{ 0,0 };
	//	inline static Vector2Base<T> up{ 0,-1 };

	T x{}, y{};

	Vector2Base() noexcept
	{
	}

	Vector2Base(T _x, T _y) noexcept : x(_x), y(_y)
	{
	}

	Vector2Base(Vector2Base const& _other) noexcept : x(_other.x), y(_other.y)
	{
	}

	Vector2Base(Vector2Base const&& _other) noexcept : x(_other.x), y(_other.y)
	{
	}

	template<typename O>
	Vector2Base(Vector2Base<O> const& _other) noexcept : x(_other.x), y(_other.y)
	{
	}

	template<typename OX, typename OY>
	Vector2Base(std::pair<OX, OY> const& _other) noexcept : x(_other.first), y(_other.second)
	{
	}

	//template<typename O>
	//Vector2Base(std::initializer_list<O> const& _other) noexcept : x(0), y(0)
	//{
	//	auto it = _other.begin();
	//	assert(it != _other.end());
	//	x = *it;
	//	++it;
	//	assert(it != _other.end());
	//	y = *it;
	//}

	//template<size_t Index>
	//constexpr const T& get() { return 0; }

	//template<>
	//constexpr const T& get<0>() { return x; }

	//template<>
	//constexpr const T& get<1>() { return y; }

	__forceinline T GetX() const { return x; }
	__forceinline T GetY() const { return y; }

	__forceinline void SetX(T _x) { x = _x; }
	__forceinline void SetY(T _y) { y = _y; }

	__forceinline void AddToX(T _x) { x += _x; }
	__forceinline void AddToY(T _y) { y += _y; }

	__forceinline T Mag() const { return sqrt(x * x + y * y); }
	__forceinline T MagSq() const { return x * x + y * y; }

	__forceinline Vector2Base Normalised() { return *this / Mag(); }

	__forceinline void Normalise() { *this /= Mag(); }
	__forceinline void SetLength(T _newlength) { *this *= (_newlength / Mag()); }

	__forceinline void operator += (Vector2Base const& _param)
	{
		x += _param.GetX();
		y += _param.GetY();
	}

	__forceinline void operator -= (Vector2Base const& _param)
	{
		x -= _param.GetX();
		y -= _param.GetY();
	}

	__forceinline void operator = (Vector2Base const& _param)
	{
		x = _param.GetX();
		y = _param.GetY();
	}

	bool operator == (Vector2Base const& other)
	{
		return x == other.x && y == other.y;
	}

	bool operator != (Vector2Base const& other)
	{
		return x != other.x || y != other.y;
	}

	operator std::tuple<T&, T&>()
	{
		return { x, y };
	}

	__forceinline Vector2Base operator + (Vector2Base const& _other) const
	{
		return Vector2Base(x + _other.GetX(), y + _other.GetY());
	}

	__forceinline Vector2Base operator - (Vector2Base const& _other) const
	{
		return Vector2Base(x - _other.GetX(), y - _other.GetY());
	}

	__forceinline Vector2Base operator - () const
	{
		return Vector2Base(-x, -y);
	}

	__forceinline Vector2Base operator * (T _mult) const
	{
		return Vector2Base(x * _mult, y * _mult);
	}

	__forceinline Vector2Base operator / (T _div) const
	{
		return Vector2Base(x / _div, y / _div);
	}

	__forceinline void operator *= (T _scale)
	{
		x *= _scale;
		y *= _scale;
	}

	__forceinline void operator /= (T _scale)
	{
		_scale = 1 / _scale;
		x *= _scale;
		y *= _scale;
	}

	// Dot product
	__forceinline T operator * (Vector2Base const& _mult) const
	{
		return (x * _mult.GetX()) + (y * _mult.GetY());
	}

	// Rotate anticlockwise
	__forceinline Vector2Base const& Rotate(T _angle)
	{
		T s = (T)sin(_angle);
		T c = (T)cos(_angle);

		T t = c * x + s * y;
		y = -s * x + c * y;
		x = t;

		return *this;
	}

	// Rotate anticlockwise
	__forceinline Vector2Base Rotated(T _angle)
	{
		Vector2Base v = *this;

		T s = (T)sin(_angle);
		T c = (T)cos(_angle);

		T t = c * v.x + s * v.y;
		v.y = -s * x + c * v.y;
		v.x = t;

		return v;
	}

	std::string toString()
	{
		std::ostringstream os;
		os << *this;
		return os.str();
	}
};

template<typename T>
Vector2Base<T> operator * (T _mult, Vector2Base<T> const& rhs)
{
	return Vector2Base<T>(rhs.x * _mult, rhs.y * _mult);
}

template<typename T>
bool operator == (Vector2Base<T> const& first, Vector2Base<T> const& second)
{
	return first.x == second.x && first.y == second.y;
}

template<typename T>
std::ostream& operator<<(std::ostream& os, Vector2Base<T> const& v)
{
	os << "{" << static_cast<T>(v.x) << "," << static_cast<T>(v.y) << "}";
	return os;
}

namespace std
{
	template<> struct hash<Vector2Base<int>>
	{
		std::size_t operator()(Vector2Base<int> const& v) const noexcept
		{
			return std::hash<int>()(v.x) ^ std::hash<int>()(v.y);
		}
	};
}

using Vector2 = Vector2Base<float>;
