/*
================================
Class that handles custom menu strings
and other stuff
================================
*/

#include "stdafx.h"
#include "Hooking.h"

#include <direct.h>

using namespace System;

#pragma unmanaged

typedef struct stringTable_s {
	char* fileName;
	int columns;
	int rows;
	char** data;
} stringTable_t;

StompHook stringHook;
DWORD stringHookLoc = 0x424960;

typedef void* (__cdecl * LoadAssetOfType_t)(int type, const char* filename);
extern LoadAssetOfType_t LoadAssetOfType;// = (LoadAssetOfType_t)0x493FB0;

typedef int (__cdecl * FS_ReadFile_t)(const char* qpath, void** buffer);
extern FS_ReadFile_t FS_ReadFile;

char tempBuffer[512];

//std::map<const char*, stringTable_t*> parsedTables;

stringTable_t* hashTable[1000];

void DumpStringTableType(stringTable_t* type);

typedef int (__cdecl * StringTableHashIW_t)(char* data);
StringTableHashIW_t StringTableHashIW = (StringTableHashIW_t)0x4F8590;

int StringTableHash(char* data)
{
	int hash = 0;

	while (*data != 0)
	{
		hash = tolower(*data) + (31 * hash);

		data++;
	}

	return hash;
}

unsigned int StringTableHashU(char* data)
{
	unsigned int hash = 0;

	while (*data != 0)
	{
		hash = tolower(*data) + (31 * hash);

		data++;
	}

	return hash;
}

// copied from IDA due to complexity, it's some kind of double hash
unsigned int AssetKeyHash(int type, char* name)
{
	/*char cur;
	unsigned int hash = type;

	for ( const char* i = name; ; i++ )
	{
		// weirdly decompiled code for making '\' hash like '/'
		while ( 1 )
		{
			cur = tolower(*i);
			if ( cur != 92 ) // '\'
				break;
			hash = 31 * hash + 47; // '/'
			i++;
		}
		if ( !cur )
			break;
		hash = cur + 31 * hash;
	}
	// infinitely weird code
	//return StringTableHash(name) % 1000;//hash % (sizeof(hashTable) / sizeof(stringTable_t*));
	return hash % (sizeof(hashTable) / sizeof(stringTable_t*));*/

	return StringTableHashU(name) % 1000;
}


#pragma managed

void StringHookFunc(char* filename, stringTable_t** table)
{
	int length = FS_ReadFile(filename, NULL);

	if (length > 0)
	{
		char* stringFile;

		FS_ReadFile(filename, (void**)&stringFile);

		stringTable_t* newTable = (stringTable_t*)malloc(sizeof(stringTable_t));
		newTable->fileName = (char*)filename;

		int rows = 0;
		int columns = 0;
		int ri = 0;

		String^ fileData = gcnew String(stringFile);
		cli::array<String^>^ lines = fileData->Replace("\r\n", "\n")->Trim()->Split('\n');
		rows = lines->Length;

		for each (String^ line in lines)
		{
			int ci = 0;
			cli::array<String^>^ datas = line->Split(',');

			if (ri == 0)
			{
				columns = datas->Length;

				newTable->data = (char**)malloc(sizeof(char*) * rows * columns * 2);
			}

			for each (String^ column in datas)
			{
				if (ci >= columns) continue;

				// NOTE: WE DON'T CLEAN IT UP HERE, OR ANYWHERE. MEMORY WILL 'LEAK', BUT THAT'S IW'S FAULT
				char* ptr = (char*)Runtime::InteropServices::Marshal::StringToHGlobalAnsi(column).ToPointer();
				int my = StringTableHash(ptr);
				newTable->data[((ri * columns) + ci) * 2] = ptr;
				newTable->data[(((ri * columns) + ci) * 2) + 1] = (char*)my;

				ci++;
			}

			ri++;
		}

		newTable->columns = columns;
		newTable->rows = rows;

		//parsedTables[filename] = newTable;
		unsigned int hash = AssetKeyHash(0x25, filename);
		hashTable[hash] = newTable;
		*table = newTable;

		//DumpStringTableType(newTable);
		return;
	}

	stringTable_t* data = (stringTable_t*)LoadAssetOfType(0x25, filename);
	*table = data;

	unsigned int hash = AssetKeyHash(0x25, filename);
	hashTable[hash] = data;
}

#pragma unmanaged

// CLR thunk
void StringHookStubFunc(char* filename, stringTable_t** table)
{
	char* name = strrchr(filename, '/');

	if (!name || stricmp(name + 1, "challengetable.csv") != 0)
	{
		*table = (stringTable_t*)LoadAssetOfType(0x25, filename);
		return;
	}

	unsigned int hash = AssetKeyHash(0x25, filename);

	if (hashTable[hash])
	{
		*table = hashTable[hash];
		return;
	}

	StringHookFunc(filename, table);
}

void __declspec(naked) StringHookStub()
{
	__asm jmp StringHookStubFunc
}

StompHook localizeStringHook;
DWORD localizeStringHookLoc = 0x4247D0;
DWORD localizeStringHookLocRet = 0x4247D6;

const char* LocalizeString(const char* name)
{
	if (!strcmp(name, "XBOXLIVE_SERVICENAME"))
	{
		return "alterIWnet";
	}

	/*if (!strcmp(name, "PLATFORM_PLAY_ONLINE_CAPS"))
	{
		//return "EE WEE NET";
		return "PLAY ONLINE";
	}*/

	if (!strcmp(name, "MENU_INVITE_CAPS"))
	{
		return "SERVER LIST";
	}

	// Menu: noprofilewarning_systemlink_create
	if (!strcmp(name, "PLATFORM_PLAYER_NOT_SIGNED_IN_OFFLINE"))
	{
		return "You are not signed in to Flux. Please run Flux, sign in and then return to the game.";
	}

	if (!strcmp(name, "MPUI_DESC_PARTY_INVITE"))
	{
		return "View a listing of dedicated servers.";
	}

	if (!strcmp(name, "MENU_DESC_INVITE_FRIENDS"))
	{
		return "View a listing of dedicated servers.";
	}

	/*if (!strcmp(name, "PLAYERCARDS_TITLE_SUBMIT_TO_AUTHORITY"))
	{
		return "Can Appeal";
	}

	if (!strcmp(name, "PLAYERCARDS_TITLE_K_FACTOR"))
	{
		return "Max Damage";
	}

	if (!strcmp(name, "PLAYERCARDS_TITLE_SPYGAME"))
	{
		return "Get Authed";
	}

	if (!strcmp(name, "PLAYERCARDS_TITLE_SSDD"))
	{
		return "<3 alterIWnet";
	}

	if (!strcmp(name, "PLAYERCARDS_TITLE_PRESTIGE10"))
	{
		return "Easy Accounting";
	}

	if (!strcmp(name, "PLAYERCARDS_TITLE_COPPERFIELD"))
	{
		return "Obv Insignia";
	}

	if (!strcmp(name, "PLAYERCARDS_TITLE_REMOTEVIEWER"))
	{
		return "Hacking Walls";
	}

	if (!strcmp(name, "PLAYERCARDS_TITLE_NTAUTHORITY"))
	{
		return "Grand Master";
	}*/

	return NULL;
}

void __declspec(naked) LocalizeStringHookStub()
{
	__asm
	{
		mov eax, [esp + 4h]
		push eax
		call LocalizeString
		add esp, 4h
		test eax, eax
		jnz returnValue

		mov eax, [esp + 8h]
		test eax, eax
		jmp localizeStringHookLocRet
returnValue:
		retn
	}
}

StompHook localizeExeStringHook;
DWORD localizeExeStringHookLoc = 0x4AE4B0;
DWORD localizeExeStringHookLocRet = 0x4AE4B5;

const char* LocalizeExeString(const char* name)
{
	if (stricmp(name, "EXE_SAYIRC") == 0)
	{
		return "say_irc";
	}

	if (stricmp(name, "EXE_SAYCONSOLE") == 0)
	{
		return "exec";
	}

	return NULL;
}

DWORD dw632 = (DWORD)0x632020C;

void __declspec(naked) LocalizeExeStringHookStub()
{
	__asm
	{
		mov eax, [esp + 4h]
		push eax
		call LocalizeExeString
		add esp, 4h
		test eax, eax
		jnz returnValue

		mov eax, dw632
		mov eax, [eax]
		jmp localizeExeStringHookLocRet
returnValue:
		retn
	}
}

CallHook testPrintHook;
DWORD testPrintHookLoc = 0x42F0C3;

typedef void (__cdecl * SomePrint_t)(int a2, signed int a3, int a4, int a5, char a6, signed int a7);
SomePrint_t SomePrint = (SomePrint_t)0x5A8860;

char* message;

void TestPrintHookFunc(int a2, signed int a3, int a4, int a5, char a6, signed int a7)
{
	__asm
	{
		mov message, eax
	}

	OutputDebugStringA(message);
	OutputDebugStringA("\n");

	__asm
	{
		mov eax, message
	}

	SomePrint(a2, a3, a4, a5, a6, a7);
}

void __declspec(naked) TestPrintHookStub()
{
	__asm jmp TestPrintHookFunc
}

void StringLingInit()
{
	memset(hashTable, 0, sizeof(hashTable));

	//testPrintHook.initialize("aaaaa", (PBYTE)testPrintHookLoc);
	//testPrintHook.installHook(TestPrintHookStub, false);

	//strcpy((char*)0x71D50C, "ui/test_menus.txt");

	stringHook.initialize("aaaaa", 5, (PBYTE)stringHookLoc);
	stringHook.installHook(StringHookStub, true, false);

	localizeStringHook.initialize("aaaaa", 6, (PBYTE)localizeStringHookLoc);
	localizeStringHook.installHook(LocalizeStringHookStub, true, false);

	localizeExeStringHook.initialize("aaaaa", 5, (PBYTE)localizeExeStringHookLoc);
	localizeExeStringHook.installHook(LocalizeExeStringHookStub, true, false);
}