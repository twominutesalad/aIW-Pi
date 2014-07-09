#pragma once

#define _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_DEPRECATE
# pragma warning(disable : 4996)

#define lINFO 0
#define lWARN 1
#define lERROR 2
#define lSAPI 3
#define lDEBUG 4

// Windows headers
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

// C/C++ headers
#include <map>
#include "iniReader.h"

int  RPCexec_getUSstate(char *url, char *func, int state);			// for testing only
void Logger(unsigned int lvl, char* caller, char* logline, ...);
