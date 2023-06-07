#include <concrt.h>
#include <ppltasks.h>

#include <sys\stat.h>

#include <map>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <vector>
#include <sstream>

#include <stdarg.h>

#include <allegro5/allegro.h>

#include "ARGUtils.h"

#define DEBUG_ENABLED 1

using namespace std;

ofstream g_logFile;

extern unsigned int g_advanceCounter;

static char buf[1024];

//static vector<string> dbgOutput;

static concurrency::critical_section g_dbgMutex;

void argDebug(string const& _str)
{
	// Warning!
	// OutputDebugString seems to be unable to keep up with the volume of text in certain circumstances, and will thus
	// skip text or fail to output it at all. Always check the log file, which has no such known problems.
#if DEBUG_ENABLED
	concurrency::critical_section::scoped_lock lock(g_dbgMutex);	// needed? http://stackoverflow.com/questions/22539282/is-msvcrts-implementation-of-fprintf-thread-safe

	//sprintf_s(buf, "%d: ", g_advanceCounter);
	//OutputDebugString( string(buf + _str + "\n").c_str() );

	if (!g_logFile.is_open())
	{
		g_logFile.open("argcore.log");
	}

	//fprintf_s(g_logFile, string(buf + _str + "\n").c_str());
	//fflush(g_logFile);

	// https://social.msdn.microsoft.com/Forums/vstudio/en-US/9244b4ae-030d-475b-b31f-8da6794f884e/how-to-get-the-index-of-virtual-processor-or-the-context-in-ppl?forum=parallelcppnative
	// GetVirtualProcessorId probably not reliable
	//const signed id = (signed)concurrency::Context::CurrentContext()->GetVirtualProcessorId();

	//g_logFile << g_advanceCounter << ":" << std::setw(2) << id << ": " << _str << endl << flush;
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

void argDebugfT(int _task, string _str, ...)	// can't use & with varargs
{
	std::stringstream ostr;
	ostr << std::setw(2) << _task << ": ";

	va_list args;
	va_start(args, _str);
	vsprintf_s(buf, _str.c_str(), args);
	va_end(args);

	ostr << buf;
	argDebug(ostr.str());
}

string stringFormat(string _str, ...)
{
	va_list args;
	va_start(args, _str);
	vsprintf_s(buf, _str.c_str(), args);
	va_end(args);
	return buf;
}

/*
bool argKeypressed(int _key)
{
	static ALLEGRO_KEYBOARD_STATE state;
	static std::map<int, bool> keyMap;
	al_get_keyboard_state(&state);
	if (!al_key_down(&state, _key) && keyMap[_key] == true)
	{
		keyMap[_key] = false;
		return true;
	}

	keyMap[_key] = al_key_down(&state, _key);
	return false;
}
*/

// From techbytes.ca
bool argFileExists(char* _strFilename)
{
	struct stat stFileInfo;
	bool blnReturn;
	int intStat;

	// Attempt to get the file attributes
	intStat = stat(_strFilename, &stFileInfo);
	if (intStat == 0) {
		// We were able to get the file attributes
		// so the file obviously exists.
		blnReturn = true;
	}
	else {
		// We were not able to get the file attributes.
		// This may mean that we don't have permission to
		// access the folder which contains this file. If you
		// need to do that level of checking, lookup the
		// return values of stat which will give you
		// more details on why stat failed.
		blnReturn = false;
	}

	return(blnReturn);
}
