#ifndef __G_DIRECTORY_H__
#define __G_DIRECTORY_H__

#include <windows.h>
#include <time.h>
#include <fstream>
#include <Psapi.h>
using namespace std;

#pragma comment( lib, "Psapi.lib" )

#ifndef DEBUGGING_ENABLED
#define DEBUGGING_ENABLED
#endif

class cDirectory
{
public:
	char*		GetDirectoryFileA( char *szFile );
	wchar_t*	GetDirectoryFileW( wchar_t *szFile );
	
	HMODULE		m_hSelf;
	MODULEINFO	m_hMISelf;
	ofstream	ofile;	
	char		dlldir[ 320 ];
};

extern cDirectory g_Directory;

#endif // __G_DIRECTORY_H__