#include <map>
#include <iostream>
#include <fstream>
//#include <vector>
#include <sstream>
#include <mutex>

#include <allegro5/allegro.h>

#include "ARGUtils.h"

#define DEBUG_ENABLED 1
#define USE_OUTPUTDEBUGSTRING 0

#if USE_OUTPUTDEBUGSTRING
#include "windows.h"
#endif

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

	if (!g_logFile.is_open())
	{
		g_logFile.open("argcore.log");
	}

	g_logFile << g_advanceCounter << ": " << _str << endl << flush;

#if USE_OUTPUTDEBUGSTRING
	ostringstream ss;
	ss << g_advanceCounter << ": " << _str << endl;
	OutputDebugString( ss.str().c_str());
#endif

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
