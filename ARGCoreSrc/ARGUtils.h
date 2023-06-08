#pragma once

#include "Vector2.h"

#include <string>
#include <cmath>

using std::string;

static const char argUtilsBufLength = 40;
static char argUtilsBuf[argUtilsBufLength];

inline string IntToString(int _int)
{
	sprintf_s(argUtilsBuf, "%d", _int);
	return argUtilsBuf;
}

inline string FloatToString(float _f)
{
	sprintf_s(argUtilsBuf, "%f", _f);
	return argUtilsBuf;
}

// Macro to get a random integer within a specified range
#define getrandom( min, max ) ((rand() % (int)(((max)+1) - (min))) + (min))

inline float FRand(float minVal, float maxVal)
{
	return minVal + ((float)rand() / RAND_MAX) * (maxVal - minVal);
}

void argDebug(string const& _str);
void argDebugf(string _str, ...);
void argDebugfT(int _task, string _str, ...);

string stringFormat(string _str, ...);

// From techbytes.ca
bool argFileExists(char* _strFilename);
