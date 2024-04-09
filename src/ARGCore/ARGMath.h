#pragma once

#include "Vector2.h"

#include <string>
#include <cmath>

namespace ARGMath
{
	extern const double PI;
	extern const double twoPI;

	float RotationFromVector(Vector2 _vec);

	float GetRotationTowards(float _from, float _to, float _amount);
}