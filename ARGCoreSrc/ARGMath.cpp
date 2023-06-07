#include "ARGMath.h"

namespace ARGMath
{
	const double PI = std::atan(1.0) * 4;
	const double twoPI = std::atan(1.0) * 8;

	float RotationFromVector(Vector2 _vec)
	{
		Vector2 up(0.f, -1.f);
		float dotProd = _vec * up;
		float thing = std::fmin(std::fmax(-1.f, dotProd / _vec.Mag()), 1.f);
		float rot = acos(thing);
		if (isnan(rot) || isinf(rot))
		{
			rot = 0.f;
		}
		if (_vec.GetX() > 0.f)
		{
			rot = -rot;
		}
		return rot;
	}

	float GetRotationTowards(float _from, float _to, float _amount)
	{
		_from = fmod(_from, twoPI);
		_to = fmod(_to, twoPI);
		float diff = _to - _from;
		if (fabs(diff) > fabs(_amount))
		{
			return _to;
		}
		else
		{
			float otherDiff = (_to + twoPI) - _from;
			if (diff < otherDiff)
			{
				return _from + _amount;
			}
			else
			{
				return _from - _amount;
			}
		}
	}

}