#include <map>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <vector>
#include <sstream>
#include <mutex>

#include <stdarg.h>

#include <allegro5/allegro.h>

#include "ARGUtils.h"

#define DEBUG_ENABLED 1

using namespace std;

ofstream g_logFile;

extern unsigned int g_advanceCounter;

static char buf[1048576];	// todo stop using c strings

//static vector<string> dbgOutput;

static mutex g_logMutex;

void argDebug(string const& _str)
{
	// Warning!
	// OutputDebugString seems to be unable to keep up with the volume of text in certain circumstances, and will thus
	// skip text or fail to output it at all. Always check the log file, which has no such known problems.
#if DEBUG_ENABLED
	scoped_lock lock(g_logMutex);	// needed? http://stackoverflow.com/questions/22539282/is-msvcrts-implementation-of-fprintf-thread-safe

	//sprintf_s(buf, "%d: ", g_advanceCounter);
	//OutputDebugString( string(buf + _str + "\n").c_str() );

	if (!g_logFile.is_open())
	{
		g_logFile.open("argcore.log");
	}

	g_logFile << g_advanceCounter << ": " << _str << endl << flush;

//	dbgOutput.push_back(_str);
#endif
}

void argDebugf(string _str, ...)	// can't use & with varargs
{
	va_list args;
	va_start(args, _str);
	vsprintf_s(buf, _str.c_str(), args);
	va_end (args);
	argDebug(buf);
}

string stringFormat(string _str, ...)	// can't use & with varargs
{
	va_list args;
	va_start(args, _str);
	vsprintf_s(buf, _str.c_str(), args);
	va_end(args);
	return buf;
}
