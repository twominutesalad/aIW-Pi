#include <windows.h>
#include "StdAfx.h"
#include "g_Directory.h"

cDirectory g_Directory;

char* cDirectory::GetDirectoryFileA( char *szFile )
{
	static char path[ 320 ];
	strcpy( path, dlldir );
	strcat( path, szFile );
	return path;
}

wchar_t* cDirectory::GetDirectoryFileW( wchar_t *szFile )
{
	char szFilename[ MAX_PATH ] = { 0 };

	wcstombs( szFilename, szFile, MAX_PATH );

	char *szDirectoryFile = GetDirectoryFileA( szFilename );

	static wchar_t szDirectoryFileW[ MAX_PATH ] = { 0 };

	mbstowcs( szDirectoryFileW, szDirectoryFile, MAX_PATH );

	return szDirectoryFileW;
}