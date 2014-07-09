/*
================================
Class that handles stat encryption and
decryption, reading and writing
================================
*/

#include "StdAfx.h"
#include "ui_mw2.h"
#include "ui_menu.h"
#include <direct.h>

//OSW
#include <SteamTypes.h>
#include "Steamclient.h"
#include <Interface_OSW.h>
#include <SteamAPI.h>

//Other
#include "g_Directory.h"
#include "detours.h"
#include "Callbacks.h"
#include "Hooking.h"

#pragma unmanaged

StompHook writeStatsHook;
DWORD writeStatsHookLoc = 0x682600;

void Logger(unsigned int lvl, char* caller, char* logline, ...);

const char* fileID;
bool useSteamCloud;
unsigned int bufferSize;

typedef bool (__cdecl * FS_WriteFile_t)(char* filename, char* folder, void* buffer, int size);
FS_WriteFile_t FS_WriteFile = (FS_WriteFile_t)0x4314F0;

bool WriteStatsFunc(int remote, void* buffer)
{
	// filename buffer
	char filename[256];

	// check buffer size to be valid, though it doesn't matter if we don't do TEA
	if (bufferSize > 8192 || bufferSize & 3) return false; // & 3 is for 4-byte alignment

	// ask about dumping stats
	//DumpStatsOnWrite(buffer, bufferSize);

	// sprintf a filename
	_snprintf(filename, sizeof(filename), "%s_aIW.stat", fileID);

	// and write the file
	return FS_WriteFile(filename, "players", buffer, bufferSize);
}

void __declspec(naked) WriteStatsHookStub()
{
	__asm
	{
		// prepare weird optimized parameters
		mov fileID, ecx
		mov useSteamCloud, dl
		mov bufferSize, eax

		// and jump to our normal cdecl function
		jmp WriteStatsFunc
	}
}

StompHook readStatsHook;
DWORD readStatsHookLoc = 0x682820;
DWORD oldReadStats = 0x68282A;

void __declspec(naked) OldReadStats()
{
	__asm
	{
		mov eax, [esp + 10h]
		sub esp, 118h

		jmp oldReadStats
	}
}

int ReadStatsLegacy(int useSteamCloud, char* fileID, void* buffer, int isize)
{
	__asm
	{
		push isize
		push buffer
		push fileID
		push useSteamCloud

		call OldReadStats

		add esp, 10h
	}
}

typedef int (__cdecl * FS_ReadFile_t)(const char* qpath, void** buffer);
FS_ReadFile_t FS_ReadFile = (FS_ReadFile_t)0x416980;

typedef void (__cdecl * Cmd_ExecuteString_t)(int a1, int a2, char* cmd);
extern Cmd_ExecuteString_t Cmd_ExecuteString;
void Com_BeginRedirect( char *buffer, int buffersize, void ( *flush )( char *) );
void Com_EndRedirect( void );

#define SV_OUTPUTBUF_LENGTH ( 16384 - 16 )
char sv_outputbuf[SV_OUTPUTBUF_LENGTH];

void parseFile(char* statz);

string stats;
bool writingStats = false;

void savePlayerStats( char  *outputbuf ) 
{
	stats += string(outputbuf);
	if (strstr(outputbuf, "winLossRatio"))
	{
		writingStats = true;
		char* statBuff = (char*)malloc( sizeof( char ) *(stats.length() +1) );
		strcpy( statBuff, stats.c_str() );
		parseFile(statBuff);
		writingStats = false;
	}
}

void getPlayerStats()
{
	Sleep(10000);

	if ( !writingStats )
	{
		writingStats = true;
		char* remaining;
	
		Logger(lINFO, "alterIWnet", "Retrieving player data");

		stats = "";

		Com_BeginRedirect( sv_outputbuf, SV_OUTPUTBUF_LENGTH, savePlayerStats );

		remaining = "dumpplayerdata";

		Cmd_ExecuteString( 0, 0, remaining );

		Com_EndRedirect();
	}
}

int ReadStatsFunc(int useSteamCloud, char* fileID, void* buffer, int size)
{
	// filename buffer
	char filename[256];

	// check buffer size to be valid, though it doesn't matter if we don't do TEA
	if (bufferSize > 8192 || bufferSize & 3) return 2; // & 3 is for 4-byte alignment

	// sprintf a filename
	_snprintf(filename, sizeof(filename), "%s_aIW.stat", fileID);

	// read file length/existence
	int length = FS_ReadFile(filename, 0);

	if (length == size)
	{
		void* fileData = NULL;
		FS_ReadFile(filename, &fileData);

		if (fileData)
		{
			memcpy(buffer, fileData, size);

			return 0;
		}
	}
	else if (length < 0)
	{
		// we don't have file
		// try old file
		// if won't work
		// user have no stats
		// and want play

		// mark SteamID as requiring 'legacy' code (with all its added bugginess :/ )
		//SetSteamIDLegacy(true);

		// call old function (yes, I said *call*, not *jump*)
		int result = ReadStatsLegacy(useSteamCloud, fileID, buffer, size);

		// reset legacy flag and return result
		//SetSteamIDLegacy(false);

		return result;
	}

	return 1;
}

void __declspec(naked) ReadStatsHookStub()
{
	CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)getPlayerStats, NULL, NULL, NULL);
	__asm
	{
		// jump to cdecl func
		jmp ReadStatsFunc
	}
}

typedef struct statFile_s
{
	char* buffer;
	int number;
} statFile_t;

typedef int (__cdecl * GetPersistentData_t)(char** name, int type, statFile_t* stats);
GetPersistentData_t GetPersistentData = (GetPersistentData_t)0x4A1520;

CallHook lockStatsHook;
DWORD lockStatsHookLoc = 0x60CF3A;//0x4ADDC0;
DWORD lockStatsHookRet = 0x4ADDC6;

char* oldStats;
char* newStats;

bool LockStatsFunc()
{
	// made names non-obvious to prevent tampering
	char* prestige = (char*)0x719360;//"prestige";
	char* experience = (char*)0x6DC9A0;//"experience";

	statFile_t olds;
	statFile_t news;

	olds.buffer = oldStats + 4;
	olds.number = 8188;
	
	news.buffer = newStats + 4;
	news.number = 8188;

	int oldxp = GetPersistentData(&experience, 1, &olds);
	int newxp = GetPersistentData(&experience, 1, &news);

	int difference = newxp - oldxp;

	if (difference > 200000)
	{
		return true;
	}

	int oldpr = GetPersistentData(&prestige, 1, &olds);
	int newpr = GetPersistentData(&prestige, 1, &news);

	difference = newpr - oldpr;

	if (difference > 1)
	{
		return true;
	}

	return false;
}

void __declspec(naked) LockStatsHookStub()
{
	__asm
	{
		mov eax, [esp + 4h]
		mov oldStats, eax

		mov eax, [esp + 8h]
		mov newStats, eax

		call LockStatsFunc

		test eax, eax
		jz doRegression
		retn

doRegression:
		jmp lockStatsHook.pOriginal
	}
}

void statsHook()
{
	writeStatsHook.initialize("aaaaa", 5, (PBYTE)writeStatsHookLoc);
	writeStatsHook.installHook(WriteStatsHookStub, true, false);

	readStatsHook.initialize("aaaaa", 10, (PBYTE)readStatsHookLoc);
	readStatsHook.installHook(ReadStatsHookStub, true, false);

	lockStatsHook.initialize("aaaaa", (PBYTE)lockStatsHookLoc);
	lockStatsHook.installHook(LockStatsHookStub, false);
}