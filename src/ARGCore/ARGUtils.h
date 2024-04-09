#pragma once

#include <string>

inline int getrandom(int min, int max)
{
	return (rand() % ((max + 1) - min)) + min;
}

inline float FRand(float minVal, float maxVal)
{
	return minVal + ((float)rand() / RAND_MAX) * (maxVal - minVal);
}

void argDebug(std::string const& _str);
void argDebugf(std::string _str, ...);

std::string stringFormat(std::string _str, ...);
