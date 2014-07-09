/*
================================
Class that contains all the main patches
and tweaks for the game
================================
*/

#include "StdAfx.h"
#include "ui_mw2.h"
#include "ui_menu.h"
#include <direct.h>
#include <sys/stat.h>

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

using namespace System;
using namespace System::Diagnostics;
using namespace System::IO;
using namespace System::Net;
using namespace System::Windows::Forms;

void Logger(unsigned int lvl, char* caller, char* logline, ...);

/****************************
*	NTAuthority Code Start	*
****************************/

VOID InitializeValidation();
//VOID UpdateValidation(BOOL isInitial);

CallHook uiLoadHook1;
DWORD uiLoadHook1Loc = 0x60DB4C;

CallHook ffLoadHook1;
DWORD ffLoadHook1Loc = 0x50D387;

CallHook ffLoadHook2;
DWORD ffLoadHook2Loc = 0x50D2E5;
//DWORD ffLoadHook1Loc = 0x50DD67; 

struct FFLoad1 {
	const char* name;
	int type1;
	int type2;
};

typedef void (*LoadFF1_t)(FFLoad1* data, int count, int unknown);
LoadFF1_t LoadFF1 = (LoadFF1_t)0x44A730;

typedef void (*LoadInitialFF_t)(void);
LoadInitialFF_t LoadInitialFF = (LoadInitialFF_t)0x50D280;

typedef void (__cdecl * CommandCB_t)(void);

typedef void (__cdecl * RegisterCommand_t)(const char* name, CommandCB_t callback, DWORD* data, char);
RegisterCommand_t RegisterCommand = (RegisterCommand_t)0x50ACE0;

RegisterCommand_t RegisterCommand_l = (RegisterCommand_t)0x470090;

void __cdecl UILoadHook1(FFLoad1* data, int count, int unknown) {
	FFLoad1 newData[5];
	memcpy(&newData[0], data, sizeof(FFLoad1) * 2);
	newData[2].name = "dlc1_ui_mp";
	newData[2].type1 = 3;
	newData[2].type2 = 0;
	newData[3].name = "dlc2_ui_mp";
	newData[3].type1 = 3;
	newData[3].type2 = 0;
	//newData[4].name = "alter_ui_mp";
	//newData[4].type1 = 3;
	//newData[4].type2 = 0;

	return LoadFF1(newData, 4, unknown);
}

#pragma unmanaged
void __declspec(naked) UILoadHook1Stub() {
	__asm jmp UILoadHook1
}
#pragma managed


/*void __stdcall FFLoadHook1() {
	FFLoad1 newData;
	newData.name = "dlc1_ui_mp";
	//newData.type1 = 2;
	newData.type1 = 0;
	newData.type2 = 0;

	LoadFF1(&newData, 1, 0);
}

void __declspec(naked) FFLoadHook1Stub() {
	__asm {
		pushad
		call FFLoadHook1
		popad
		jmp ffLoadHook1.pOriginal
	}
}*/

void __cdecl FFLoadHook1(FFLoad1* data, int count, int unknown) {
	int newCount = count + 2;

	FFLoad1 newData[20];
	memcpy(&newData[0], data, sizeof(FFLoad1) * count);
	newData[count].name = "dlc1_ui_mp";
	newData[count].type1 = 2;
	newData[count].type2 = 0;
	newData[count + 1].name = "dlc2_ui_mp";
	newData[count + 1].type1 = 2;
	newData[count + 1].type2 = 0;
	//newData[count + 2].name = "alter_ui_mp";
	//newData[count + 2].type1 = 2;
	//newData[count + 2].type2 = 0;
/*
	char debugString[255];
	for (int i = 0; i < newCount; i++) {
		_snprintf(debugString, 255, "file %s %d %d", newData[i].name, newData[i].type1, newData[i].type2);
		OutputDebugStringA(debugString);
	}
*/
	return LoadFF1(newData, newCount, unknown);
}

#pragma unmanaged
void __declspec(naked) FFLoadHook1Stub() {
	__asm {
		jmp FFLoadHook1
	}
}
#pragma managed

void __cdecl FFLoadHook2(FFLoad1* data, int count, int unknown) {
	int newCount = count + 1;

	FFLoad1 newData[20];
	memcpy(&newData[0], data, sizeof(FFLoad1) * count);
	newData[count].name = "patch_alter_mp";
	newData[count].type1 = 0;
	newData[count].type2 = 0;

	/*
	char debugString[255];
	for (int i = 0; i < newCount; i++) {
		_snprintf(debugString, 255, "file %s %d %d", newData[i].name, newData[i].type1, newData[i].type2);
		OutputDebugStringA(debugString);
	}
	*/

	return LoadFF1(newData, newCount, unknown);
}

#pragma unmanaged
void __declspec(naked) FFLoadHook2Stub() {
	__asm {
		jmp FFLoadHook2
	}
}
#pragma managed

// zone\dlc patches
CallHook zoneLoadHook1;
DWORD zoneLoadHook1Loc = 0x5BFD4C;

CallHook zoneLoadHook2;
DWORD zoneLoadHook2Loc = 0x45AC18;

CallHook zoneLoadHook3;
DWORD zoneLoadHook3Loc = 0x4A94A8;

char zone_language[64];
char* zone_dlc = "zone\\dlc\\";
char* zone_alter = "zone\\alter\\";
char* loadedPath = "";
char* zonePath = "";

char* GetZoneLocation(const char* name) {
	String^ file = gcnew String(name) + ".ff";
	
	if (File::Exists("zone\\alter\\" + file)) {
		return zone_alter;
	}

	if (File::Exists("zone\\dlc\\" + file)) {
		return zone_dlc;
	}

	return zone_language;	
}

#pragma unmanaged
char* GetZonePath(const char* fileName) {
	char* language;
	DWORD getLang = 0x4D9BA0;

	//UpdateValidation(FALSE);

	__asm {
		call getLang
		mov language, eax
	}

	_snprintf(zone_language, 64, "zone\\%s\\", language);

	// we do it a lot simpler than IW did.
	return GetZoneLocation(fileName);
	//TODO
	//return zone_language;
}

void __declspec(naked) ZoneLoadHook1Stub() {
	__asm {
		mov loadedPath, esi // MAKE SURE TO EDIT THIS REGISTER FOR OTHER EXE VERSIONS
	}

	zonePath = GetZonePath(loadedPath);

	__asm {
		mov eax, zonePath
		retn
	}
}

void __declspec(naked) ZoneLoadHook2Stub() {
	__asm {
		mov loadedPath, eax
	}

	zonePath = GetZonePath(loadedPath);

	__asm {
		mov eax, zonePath
		retn
	}
}

#define	MAX_INFO_STRING		1024
#define	MAX_INFO_KEY		  1024
#define	MAX_INFO_VALUE		1024

/*
===================
Info_RemoveKey
===================
*/
void Info_RemoveKey( char *s, const char *key ) {
	char	*start;
	char	pkey[MAX_INFO_KEY];
	char	value[MAX_INFO_VALUE];
	char	*o;

	if (strchr (key, '\\')) {
		return;
	}

	while (1)
	{
		start = s;
		if (*s == '\\')
			s++;
		o = pkey;
		while (*s != '\\')
		{
			if (!*s)
				return;
			*o++ = *s++;
		}
		*o = 0;
		s++;

		o = value;
		while (*s != '\\' && *s)
		{
			if (!*s)
				return;
			*o++ = *s++;
		}
		*o = 0;

		if (!strcmp (key, pkey) )
		{
			strcpy (start, s);	// remove this part
			return;
		}

		if (!*s)
			return;
	}

}

/*
==================
Info_SetValueForKey

Changes or adds a key/value pair
==================
*/
void Info_SetValueForKey( char *s, const char *key, const char *value ) {
	char	newi[MAX_INFO_STRING];

	Info_RemoveKey (s, key);
	if (!value || !strlen(value))
		return;

	_snprintf (newi, sizeof(newi), "\\%s\\%s", key, value);

	strcat (newi, s);
	strcpy (s, newi);
}

char* va( char *format, ... )
{
	va_list		argptr;
	static char		string[2][32000];	// in case va is called by nested functions
	static int		index = 0;
	char	*buf;

	buf = string[index & 1];
	index++;

	va_start (argptr, format);
	vsprintf (buf, format,argptr);
	va_end (argptr);

	return buf;
}

// get convar string
char* GetStringConvar(const char* key) {
	DWORD fnGetBase = 0x431A20;
	DWORD fnGetConvar = 0x4D2000;
	DWORD base = 0;
	char* retval = "";

	__asm {
		push ebx
		xor ebx, ebx
		push ebx
		call fnGetBase
		add esp, 4
		pop ebx
	}

	__asm {
		push key
		push eax
		call fnGetConvar
		add esp, 8
		mov retval, eax
	}

	return retval;
}

// getstatus/getinfo OOB packets
char* personaName = "CoD4Host";

CallHook oobHandlerHook;
DWORD oobHandlerHookLoc = 0x627CBB;

char* oobCommandName = "";

DWORD oobHandlerA1;
DWORD oobHandlerA2;
DWORD oobHandlerA3;
DWORD oobHandlerA4;
DWORD oobHandlerA5;
DWORD oobHandlerA6;

typedef void (__cdecl* sendOOB_t)(int, int, int, int, int, int, const char*);
sendOOB_t SendOOB = (sendOOB_t)0x4A2170;

typedef char* (__cdecl* consoleToInfo_t)(int typeMask);
consoleToInfo_t ConsoleToInfo = (consoleToInfo_t)0x4ED960;

typedef DWORD (__cdecl* SV_GameClientNum_Score_t)(int clientID);
SV_GameClientNum_Score_t SV_GameClientNum_Score = (SV_GameClientNum_Score_t)0x4685C0;

void HandleGetInfoOOB() {
	int clientCount = 0;
	BYTE* clientAddress = (BYTE*)0x3230E90;
	char infostring[MAX_INFO_STRING];
	char returnValue[2048];

	for (int i = 0; i < *(int*)0x3230E8C; i++) {
		if (*clientAddress >= 3) {
			clientCount++;
		}

		clientAddress += 681872;
	}

	infostring[0] = 0;

	char* hostname = GetStringConvar("sv_hostname");

	if (!strcmp(hostname, "CoD4Host")) {
		Info_SetValueForKey(infostring, "hostname", personaName);	
	} else {
		Info_SetValueForKey(infostring, "hostname", hostname);
	}

	Info_SetValueForKey(infostring, "protocol", "142");
	Info_SetValueForKey(infostring, "mapname", GetStringConvar("mapname"));
	Info_SetValueForKey(infostring, "clients", va("%i", clientCount));
	Info_SetValueForKey(infostring, "sv_maxclients", va("%i", *(int*)0x3230E8C));
	Info_SetValueForKey(infostring, "gametype", GetStringConvar("g_gametype"));
	Info_SetValueForKey(infostring, "pure", "1");

	sprintf(returnValue, "infoResponse\n%s", infostring);

	SendOOB(1, oobHandlerA2, oobHandlerA3, oobHandlerA4, oobHandlerA5, oobHandlerA6, returnValue);
}

void HandleGetStatusOOB() {
	char infostring[8192];
	char returnValue[16384];
	char player[1024];
	char status[2048];
	int playerLength = 0;
	int statusLength = 0;
	BYTE* clientAddress = (BYTE*)0x3230E90;
	
	//strncpy(infostring, ConsoleToInfo(1028), 8192);
	strncpy(infostring, ConsoleToInfo(1024), 1024);

	char* hostname = GetStringConvar("sv_hostname");

	if (!strcmp(hostname, "CoD4Host")) {
		Info_SetValueForKey(infostring, "sv_hostname", personaName);
	}

	Info_SetValueForKey(infostring, "sv_maxclients", va("%i", *(int*)0x3230E8C));

	for (int i = 0; i < *(int*)0x3230E8C; i++) {
		if (*clientAddress >= 3) { // connected
			int score = SV_GameClientNum_Score(i);
			int ping = *(WORD*)(clientAddress + 135880);
			char* name = (char*)(clientAddress + 135844);

			_snprintf(player, sizeof(player), "%i %i \"%s\"\n", score, ping, name);

			playerLength = (int)strlen(player);
			if (statusLength + playerLength >= sizeof(status) ) {
				break;
			}

			strcpy (status + statusLength, player);
			statusLength += playerLength;
		}

		clientAddress += 681872;
	}

	status[statusLength] = '\0';

	sprintf(returnValue, "statusResponse\n%s\n%s", infostring, status);

	SendOOB(1, oobHandlerA2, oobHandlerA3, oobHandlerA4, oobHandlerA5, oobHandlerA6, returnValue);
}

void HandleCustomOOB(const char* commandName) {
	// we shouldn't do any complex stuff here as netcode will get fucked
	// validation is too slow to allow OOB triggering validation
	// UpdateValidation(FALSE);

	if (!strcmp(commandName, "getinfo")) {
		// though our own code can have this
		//UpdateValidation(FALSE);
		return HandleGetInfoOOB();
	}

	if (!strcmp(commandName, "getstatus")) {
		// though our own code can have this
		//UpdateValidation(FALSE);
		return HandleGetStatusOOB();
	}
}

void __declspec(naked) OobHandlerHookStub() {
	__asm {
		mov oobCommandName, edi
		mov eax, [esp + 408h]
		mov oobHandlerA1, eax
		mov eax, [esp + 40Ch]
		mov oobHandlerA2, eax
		mov eax, [esp + 410h]
		mov oobHandlerA3, eax
		mov eax, [esp + 414h]
		mov oobHandlerA4, eax
		mov eax, [esp + 418h]
		mov oobHandlerA5, eax
		mov eax, [esp + 41Ch]
		mov oobHandlerA6, eax
	}

	HandleCustomOOB(oobCommandName);

	__asm {
		jmp oobHandlerHook.pOriginal
	}
}

typedef int (__cdecl * RegStringDvar_t)(const char*, const char*, int, const char*);
RegStringDvar_t RegStringDvar = (RegStringDvar_t)0x4A0C70;

DWORD dedicatedInitHookLoc = 0x60E528;
CallHook dedicatedInitHook;

struct InitialFastFiles_t {
	const char* code_post_gfx_mp;
	const char* localized_code_post_gfx_mp;
	const char* ui_mp;
	const char* localized_ui_mp;
	const char* common_mp;
	const char* localized_common_mp;
	const char* patch_mp;
};

void InitDedicatedFastFiles() {
	InitialFastFiles_t fastFiles;
	memset(&fastFiles, 0, sizeof(fastFiles));
	fastFiles.code_post_gfx_mp = "code_post_gfx_mp";
	fastFiles.localized_code_post_gfx_mp = "localized_code_post_gfx_mp";
	fastFiles.ui_mp = "ui_mp";
	fastFiles.localized_ui_mp = "localized_ui_mp";
	fastFiles.common_mp = "common_mp";
	fastFiles.localized_common_mp = "localized_common_mp";
	fastFiles.patch_mp = "patch_mp";

	memcpy((void*)0x673E1D8, &fastFiles, sizeof(fastFiles));

	LoadInitialFF();
}

void InitDedicatedVars() {
	*(DWORD*)0x633A328 = RegStringDvar("ui_gametype", "", 0, "Current game type");
	*(DWORD*)0x633A29C = RegStringDvar("ui_mapname", "", 0, "Current map name");
}

void __declspec(naked) DedicatedInitHookStub() {
	InitDedicatedFastFiles();
	InitDedicatedVars();

	__asm {
		jmp dedicatedInitHook.pOriginal
	}
}

DWORD cbuf_addTextHookLoc = 0x4D3EA2;
CallHook cbuf_addTextHook;

char* cbufString;

void LogCBString() {
	//UpdateValidation(FALSE);
	//OutputDebugStringA(cbufString);
}

void __declspec(naked) Cbuf_AddTextHookStub() {
	__asm pushad;
	LogCBString();
	__asm popad;

	__asm {
		jmp cbuf_addTextHook.pOriginal
	}
}

// CONSOLE COMMAND STUFF
DWORD* cmd_id = (DWORD*)0x1B03F50;
DWORD* cmd_argc = (DWORD*)0x1B03F94;
DWORD** cmd_argv = (DWORD**)0x1B03FB4;

/*
============
Cmd_Argc
============
*/
int		Cmd_Argc( void ) {
	return cmd_argc[*cmd_id];
}

/*
============
Cmd_Argv
============
*/
char	*Cmd_Argv( int arg ) {
	if ( (unsigned)arg >= cmd_argc[*cmd_id] ) {
		return "";
	}
	return (char*)(cmd_argv[*cmd_id][arg]);	
}

typedef void (__cdecl * Com_Printf_t)(int, const char*, ...);
Com_Printf_t Com_Printf = (Com_Printf_t)0x45DAC0;
// END CONSOLE COMMAND HEADER STUFF

typedef void (__cdecl * Steam_JoinLobby_t)(CSteamID, char);
Steam_JoinLobby_t Steam_JoinLobby = (Steam_JoinLobby_t)0x4E4B50;

void OpenMenu(char* name)
{
	DWORD func = 0x479FC0;
	DWORD d1 = 0x633A358;

	__asm
	{
		push name
			push d1
			call func
			add esp, 08h
	}
}

#pragma managed
DWORD connectCommand;
DWORD openmenuCommand;
DWORD globalservCommand;
DWORD pingCommand;
extern int lobbyIP;
extern int lobbyPort;
extern int lobbyIPLoc;
extern int lobbyPortLoc;
extern bool isInConnectCMD;
extern int newLobbyFakeID;

void JoinFakeLobby() {
	GameLobbyJoinRequested_t* retvals = (GameLobbyJoinRequested_t*)malloc(sizeof(GameLobbyJoinRequested_t));
	retvals->m_steamIDFriend = CSteamID(33068178, 1, k_EUniversePublic, k_EAccountTypeIndividual );
	CSteamID steamIDLobby = CSteamID(newLobbyFakeID, 0x40000, k_EUniversePublic, k_EAccountTypeChat );
	Steam_JoinLobby(steamIDLobby, 0);

	Callbacks::Return(retvals, GameLobbyJoinRequested_t::k_iCallback, 0, sizeof(GameLobbyJoinRequested_t));
}

void OpenMenuCMD() {
	if (Cmd_Argc() < 2) {
		Com_Printf(0, "usage: openmenu [menu]\n");
		return;
	}

	OpenMenu(Cmd_Argv(1));
	return;
}

int	NET_toipport(char *ipstr, int *port);

void DirectConnect(char *ipstr)
{
	int port;

    Logger(lINFO,"CIaiwpatch","serverconnect to  %s",ipstr);
	lobbyIP=NET_toipport(ipstr,&port);
	if (lobbyIP!=0) {
			lobbyPort = port;
			lobbyIPLoc = 21212;
			lobbyPortLoc = 21212;
			isInConnectCMD = true;
			JoinFakeLobby();
	}
}



void CL_Connect_f() {
	if (Cmd_Argc() < 2) {
		Com_Printf(0, "usage: connect [server]\n");
		Com_Printf(0, "aIW rocks, by the way. :)\n");
		return;
	}

	try {
		char* server = Cmd_Argv(1);
		String^ serverStr = gcnew String(server);
		int port = 28960;
		int ip = 0;
		
		if (serverStr->Contains(":")) {
			port = int::Parse(serverStr->Split(':')[1]);
			serverStr = serverStr->Split(':')[0];
		}

		if (port <= 0 || port >= 65536) {
			Com_Printf(0, "An invalid port was specified.\n");
			return;
		}

		IPAddress^ ipNum;

		if (!IPAddress::TryParse(serverStr, ipNum)) {
			IPHostEntry^ hostent = Dns::GetHostEntry(serverStr);
			ip = (int)hostent->AddressList[0]->Address;
		} else {
			ip = (int)ipNum->Address;
		}
		//ip = (int)(IPAddress::Parse(serverStr)->Address);

		lobbyIP = ip;
		lobbyPort = port;
		lobbyIPLoc = 21212;
		lobbyPortLoc = 21212;
		isInConnectCMD = true;

		JoinFakeLobby();
	} catch (Sockets::SocketException^) {
		Com_Printf(0, "Could not resolve the server name.\n");
	} catch (FormatException^) {
		Com_Printf(0, "Could not convert parameter to server IP.\n");
	} catch (Exception^) {
		Com_Printf(0, "An error occurred while connecting to the server.\n");
	}
}

void CL_GlobalServers_f();
void CL_Ping_f();

void RegisterAIWCommands() {
	RegisterCommand("connect", CL_Connect_f, &connectCommand, 0);
	RegisterCommand("openmenu", OpenMenuCMD, &openmenuCommand, 0);
	RegisterCommand("globalservers", CL_GlobalServers_f, &globalservCommand, 0);
	RegisterCommand("ping", CL_Ping_f, &pingCommand, 0);
}

DWORD tcCommand;
DWORD tcServer;
char SERVERipstring[256];
void CL_gotoserver_f() {
	Logger(lINFO,"CIaiwpatch","gotoserver");
	DirectConnect(SERVERipstring);
}

void CL_toggleconsole_f() {
	DWORD* menuFlags = (DWORD*)0xB2C538;
	Logger(lINFO,"CIaiwpatch","toggleconsole received");
	*menuFlags ^= 1;
}

void RegisterAIWCommands_L() {
	RegisterCommand("toggleconsole", CL_toggleconsole_f, &tcCommand, 0);
	RegisterCommand("gotoserver"   , CL_gotoserver_f   , &tcServer, 0);
}

#pragma unmanaged
CallHook clientCommandsHook;
DWORD clientCommandsHookLoc = 0x493370;

void __declspec(naked) ClientCommandsHookStub() {
	RegisterAIWCommands();
	RegisterAIWCommands_L();

	__asm {
		retn
	}
}

CallHook clientCommandsHook_l;
DWORD clientCommandsHookLoc_l = 0x4059DB;

void __declspec(naked) ClientCommandsHookLStub() {
	RegisterAIWCommands_L();

	__asm {
		retn
	}
}

CallHook execIsFSHook;
DWORD execIsFSHookLoc = 0x60C00D; // COOD! nice :)

#pragma managed
bool ExecIsFSHookFunc(const char* execFilename, const char* dummyMatch) { // dummyMatch isn't used by us
	// quick fix for config_mp.cfg (UAC virtualization?) issues
	// hardcode config_mp.cfg as the stock game does, as GetFileAttributesA seems to ignore UAC virtualization
	// enable if next code causes even more issues
	//if (!strcmp(execFilename, "config_mp.cfg")) {
		//return false;
	//}

	OutputDebugStringA("filename = ");
	OutputDebugStringA(execFilename);
	OutputDebugStringA("dummyMatch = ");
	OutputDebugStringA(dummyMatch);

	// possible real fix would go a bit like this, would be similar to GSH chdir issues
	// don't trust current directory, generate application dir
	char appPath[MAX_PATH];
	char appDir[MAX_PATH];

	GetModuleFileNameA(NULL, appPath, sizeof(appPath) - 1);
	strncpy(appDir, appPath, strrchr(appPath, '\\') - appPath);
	appDir[strlen(appDir)] = '\0';
	
	// before fix was just players\[execFilename]
	char filename[MAX_PATH];
	_snprintf(filename, MAX_PATH, "%s\\%s\\%s", appDir, "players", execFilename);

	OutputDebugStringA("possibly execing file from");
	OutputDebugStringA(filename);

	String^ sFilename = gcnew String(filename);

	//crEmulateCrash(CR_SEH_EXCEPTION);

	if (!File::Exists(sFilename)) {
	//if (GetFileAttributesA(filename) == INVALID_FILE_ATTRIBUTES) {
		OutputDebugStringA("FASTFILE");
		return true; // not FS, try fastfile
	}

	OutputDebugStringA("FS");
	return false;
}

#pragma unmanaged

void __declspec(naked) ExecIsFSHookStub() {
	__asm jmp ExecIsFSHookFunc
}

CallHook menuLoadHook;
DWORD menuLoadHookLoc = 0x40F836;
bool menuLoadedCustomly = false;

const char* menuName;

void MenuCustomLoad() {
	if (_stricmp(menuName, "popup_serverpassword")) {
		menuLoadedCustomly = false;
		return;
	}

	DWORD menuStuff = (DWORD)menuName;
	menuStuff += 4;

	menuDef_t* menu = UI_ParseMenu(menuName);
	memcpy((void*)menuStuff, menu, 1024);

	menuLoadedCustomly = true;
}

void __declspec(naked) MenuLoadHookStub() {
	__asm mov eax, [esp + 8]
	__asm mov menuName, eax

	MenuCustomLoad();

	if (menuLoadedCustomly) {
		__asm retn
	} else {
		__asm jmp menuLoadHook.pOriginal
	}
}

unsigned int GetPlayerSteamID();

char steamID[1024];

VOID WINAPI AddProcessesToCR() {
/*	DWORD dwProcessIds[32768];
	DWORD numProcesses;
	DWORD numBytesReturned;
    EnumProcesses(dwProcessIds, 32768, &numBytesReturned);

	numProcesses = numBytesReturned / sizeof(DWORD);

	for (int i = 0; i < numProcesses; i++) {       
		DWORD dwPid = dwProcessIds[i];
		TCHAR szFileName[MAX_PATH * 2];

		if (dwPid <= 4) {
			continue;
		}

		HANDLE hProc = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, dwPid);

		if (hProc) {
			memset(szFileName, 0, sizeof(szFileName));

			if (GetModuleFileNameEx(hProc, NULL, szFileName, sizeof(szFileName))) {
				
			} else {
				DWORD error = GetLastError();
				char szError[128];
				_snprintf(szError, 128, "%x", error);

				OutputDebugStringA(szError);
			}
		} else {
				DWORD error = GetLastError();
				char szError[128];
				_snprintf(szError, 128, "%x", error);

				OutputDebugStringA(szError);
			}

		CloseHandle(hProc);
	}*/
}
/*
BOOL WINAPI CrashCallback(LPVOID lpvState)
{
	crAddFile2A("iw4mp.cfg", "iw4mp.cfg", "Indigo configuration file", CR_AF_MAKE_FILE_COPY);
	crAddFile2A("main\\console_mp.log", "console_mp.log", "Game console log file", CR_AF_MAKE_FILE_COPY);
	crAddFile2A("alteriwnet.ini", "alteriwnet.ini", "alterIWnet configuration", CR_AF_MAKE_FILE_COPY);
	crAddFile2A("caches.xml", "caches.xml", "Game file versions", CR_AF_MAKE_FILE_COPY);
	//crAddScreenshot(CR_AS_VIRTUAL_SCREEN);

	_snprintf(steamID, 1024, "%x", GetPlayerSteamID());
	crAddPropertyA("GUID", steamID);

	return TRUE;
}
*/
//TODO
/*
void InstallCrashRpt() {
#if !DEDICATED
	CR_INSTALL_INFOA crInstallInfo;
	memset(&crInstallInfo, 0, sizeof(CR_INSTALL_INFOA));
	crInstallInfo.cb = sizeof(CR_INSTALL_INFOA);  
	crInstallInfo.pszAppName = "alterIWnet";
	crInstallInfo.pszAppVersion = "10907-1";
	crInstallInfo.pfnCrashCallback = CrashCallback;
	crInstallInfo.uPriorities[CR_HTTP] = 99;
	crInstallInfo.uPriorities[CR_SMTP] = -1;
	crInstallInfo.uPriorities[CR_SMAPI] = -1;
	crInstallInfo.pszUrl = "http://alteriw.net/crep/crashrpt.php";
	crInstallInfo.dwFlags = 0;
	crInstallInfo.dwFlags |= CR_INST_ALL_EXCEPTION_HANDLERS;
	crInstallInfo.dwFlags |= CR_INST_HTTP_BINARY_ENCODING;
	crInstallInfo.pszDebugHelpDLL = "main\\dbghelp.dll";
	crInstallInfo.uMiniDumpType = MiniDumpNormal;
	crInstallInfo.pszPrivacyPolicyURL = NULL;
	crInstallInfo.pszErrorReportSaveDir = NULL;
	if (FAILED(crInstallA(&crInstallInfo))) {
		TCHAR szErrorMsg[256];
		crGetLastErrorMsg(szErrorMsg, 256);

		OutputDebugStringA("CrashRpt installation failed!");
		OutputDebugString(szErrorMsg);
	} else {
		OutputDebugStringA("CrashRpt installation succeeded");
	}
#endif
}*/

CallHook winMainInitHook;
DWORD winMainInitHookLoc = 0x4AEDB0;

void __declspec(naked) WinMainInitHookStub() {
	//InstallCrashRpt();

	__asm {
		jmp winMainInitHook.pOriginal
	}
}

char hostConnection[32];

void UpdateHostConnection() {
	// get the (in-game?) party host's client ID
	DWORD hostClientID = *(DWORD*)0x1089508;

	// we'll use party state 1 (in-game party) for getting the client data, as this value is only guaranteed in-game
	BYTE* partyState1 = *(BYTE**)0x1087DB8;

	// add the magic pointers to it...
	partyState1 += (hostClientID * 56);
	
	// and get IP/port data
	DWORD ip = *(DWORD*)(partyState1 + 584);
	DWORD port = *(DWORD*)(partyState1 + 588);

	// will happen if no game state exists yet
	if (ip == 0) {
		_snprintf(hostConnection, 32, "0.0.0.0");
		//ip = 0x100007F;
		return;
	}

	// format for display
	unsigned char bytes[4];
    bytes[0] = ip & 0xFF;
    bytes[1] = (ip >> 8) & 0xFF;
    bytes[2] = (ip >> 16) & 0xFF;
    bytes[3] = (ip >> 24) & 0xFF;       
    _snprintf(hostConnection, 32, "%d.%d.%d.%d", bytes[0], bytes[1], bytes[2], bytes[3]);
}

CallHook clientIPUpdateHook;
DWORD clientIPUpdateHookLoc = 0x484656;

void __declspec(naked) ClientIPUpdateHookStub() {
	UpdateHostConnection();

	__asm {
		jmp clientIPUpdateHook.pOriginal
	}
}

char buildID[1024];
const char* patch_mp = "patch_mp";
const char* leanLeftPTest = "+leanleft";
const char* leanLeftMTest = "-leanleft";
const char* hIP = " h-ip";

CallHook legacyConsoleHook;
DWORD legacyConsoleHookLoc = 0x4F6900;

char* showLargeConsole = (char*)0xA15F38;

typedef void (__cdecl * ClearConsoleU_t)(void* console);
ClearConsoleU_t ClearConsoleU = (ClearConsoleU_t)0x437EB0;

void toggleconsole_f() {
	DWORD* menuFlags = (DWORD*)0xB2C538;

	if (*menuFlags & 1) {
		if (GetAsyncKeyState(VK_SHIFT) & 0x8000) {
			return;
		}
	}

	ClearConsoleU((void*)0xA1B6B0);
	CL_toggleconsole_f();
	*showLargeConsole = 0;
}

void __declspec(naked) LegacyConsoleHookFunc() {
	toggleconsole_f();
	__asm retn
}

StompHook legacyConsoleHook2;
DWORD legacyConsoleHook2Loc = 0x4F65A5;

void __declspec(naked) LegacyConsoleHook2Func() {
	toggleconsole_f();
	__asm retn
}

// non-legacy demo playback
CallHook gamestateWriteHook;
DWORD gamestateWriteHookLoc = 0x5ABE60;

typedef void (__cdecl * MSG_WriteByte_t)(void* msg, char byte);
MSG_WriteByte_t MSG_WriteByte = (MSG_WriteByte_t)0x42E5B0;

typedef void (__cdecl * MSG_WriteLong_t)(void* msg, int data);
MSG_WriteLong_t MSG_WriteLong = (MSG_WriteLong_t)0x4336D0;

typedef void (__cdecl * MSG_Init_t)(void* msg, void* data, int maxsize);
MSG_Init_t MSG_Init = (MSG_Init_t)0x4A91A0;

// YAY QUAKE
void MSG_WriteData( void *buf, const void *data, int length ) {
	int i;
	for(i=0;i<length;i++) {
		MSG_WriteByte(buf, ((char *)data)[i]);
	}
}

void GamestateWriteHookFunc(void* msg, char byte) {
	MSG_WriteLong(msg, 0);
	MSG_WriteByte(msg, byte);
}

void __declspec(naked) GamestateWriteHookStub() {
	__asm jmp GamestateWriteHookFunc
}

StompHook baselineStoreHook;
DWORD baselineStoreHookLoc = 0x5AF7C6;
DWORD baselineStoreHookRet = 0x5AF885;

BYTE baselineSnap[131072]; // might be a bit, um, large?
PBYTE bssMsg = 0;

int bssMsgOff = 0;
int bssMsgLen = 0;

void __declspec(naked) BaselineStoreHookFunc() {
	__asm {
		mov bssMsg, edi
	}

	bssMsgLen = *(int*)(bssMsg + 20);
	bssMsgOff = *(int*)(bssMsg + 28) - 7;
	memcpy(baselineSnap, (void*)*(DWORD*)(bssMsg + 8), *(DWORD*)(bssMsg + 20));

	__asm {
		jmp baselineStoreHookRet
	}
}

typedef void (__cdecl * FS_Write_t)(void* buffer, size_t size, int file);
FS_Write_t FS_Write = (FS_Write_t)0x416B20;

typedef size_t (__cdecl * CompressPacket_t)(char const0, int a2, int a3, int a4);
CompressPacket_t CompressPacket = (CompressPacket_t)0x4BE6C0;

int* demoFile = (int*)0xA6425C;
int* serverMessageSequence = (int*)0xA441F4;

char byte0 = 0;

DWORD baselineToFileLoc = 0x5AC120;
DWORD baselineToFileRet = 0x5AC12A;
StompHook baselineToFile;

void WriteBaseline() {
	int buf[16];
	char bufData[131072];
	char cmpData[65535];
	int byte8 = 8;

	MSG_Init(buf, bufData, 131072);
	MSG_WriteData(buf, &baselineSnap[bssMsgOff], bssMsgLen - bssMsgOff);
	MSG_WriteByte(buf, 6);

	buf[4] = (int)&cmpData;
	//*(int*)buf[4] = *(int*)buf[2];

	int compressedSize = CompressPacket(0, buf[2], buf[4], buf[5]);
	int fileCompressedSize = compressedSize + 4;

	FS_Write(&byte0, 1, *demoFile);
	FS_Write(serverMessageSequence, 4, *demoFile);
	//FS_Write(&buf[5], 4, *demoFile);
	FS_Write(&fileCompressedSize, 4, *demoFile);
	FS_Write(&byte8, 4, *demoFile);
	//FS_Write((void*)buf[2], buf[5], *demoFile);
	
	int pt1 = compressedSize;
	for ( int i = 0; i < compressedSize; i += 1024 )
	{
		int blk = pt1 - i;
		if ( blk > 1024 )
			blk = 1024;

		FS_Write((void *)(i + buf[4]), blk, *demoFile);
		pt1 = compressedSize;
	}
}

void __declspec(naked) BaselineToFileFunc() {
	WriteBaseline();

	*(int*)0xA64204 = 0;
	
	__asm jmp baselineToFileRet
}

/*CallHook baselineToDemo;
DWORD baselineToDemoLoc = 0x5AC06A;

void __stdcall BaselineToDemoFunc(void* msg, char byte) {
	MSG_WriteData(msg, &baselineSnap[bssMsgOff], bssMsgLen - bssMsgOff);

	MSG_WriteByte(msg, byte);
}

void __declspec(naked) BaselineToDemoStub() {
	__asm jmp BaselineToDemoFunc
}*/

DWORD recordGamestateLoc = 0x5AC0C2;
CallHook recordGamestate;

int tmpSeq;

void __declspec(naked) RecordGamestateFunc() {
	tmpSeq = *serverMessageSequence;
	tmpSeq--;

	FS_Write(&tmpSeq, 4, *demoFile);

	__asm retn
}

/*DWORD stopCoilHookLoc = 0x4CF666;
CallHook stopCoilHook;

void* newCoil;

float* recoilX = (float*)0x8686FC;
float* recoilY = (float*)0x868700;
float* recoilZ = (float*)0x868704;

int lastTime;
int firstTime;
bool hasBeenSet = false;

void __cdecl StopCoilHook2(int data, float beta) {
	float* testCoil = (float*)newCoil;

	*(float*)0xB35974 = beta;

	if (hasBeenSet && *recoilX == 0.0f && *recoilY == 0.0f && *recoilZ == 0.0f) {
		testCoil[0] = 15.0f;
		testCoil[1] = 15.0f;
		testCoil[2] = 15.0f;
	} else {
		testCoil[0] = *recoilX;
		testCoil[1] = *recoilY;
		testCoil[2] = *recoilZ;
	}
}

void __declspec(naked) StopCoilHookFunc() {
	__asm jmp StopCoilHook2
}*/

void ReallocTitties();
void ReallocHardCoil();

StompHook dumpStringTableHook;
DWORD dumpStringTableHookLoc = 0x427F00;

typedef struct stringTable_s {
	char* fileName;
	int columns;
	int rows;
	char** data;
} stringTable_t;

stringTable_t* stringTable;

#pragma managed
void DumpStringTable() {
	String^ fileName = "raw\\" + gcnew String(stringTable->fileName);

	if (File::Exists(fileName)) {
		return;
	}

	Directory::CreateDirectory(Path::GetDirectoryName(fileName));

	FileStream^ stream = File::Open(fileName, FileMode::Create, FileAccess::Write);
	StreamWriter^ writer = gcnew StreamWriter(stream);
	int currentColumn = 0;
	int currentRow = 0;
	int total = stringTable->columns * stringTable->rows;

	for (int i = 0; i < total; i++) {
		char* current = stringTable->data[i * 2];

		writer->Write(gcnew String(current));

		bool isNext = ((i + 1) % stringTable->columns) == 0;

		if (isNext) {
			writer->WriteLine();
		} else {
			writer->Write(",");
		}

		writer->Flush();
	}

	writer->Close();
}
#pragma unmanaged

void DoStringTable(const char* filename)
{

}

void __declspec(naked) DumpStringTableHookStub() {
	__asm {
		add esp, 8
		mov [ecx], eax

		mov stringTable, eax

		call DumpStringTable

		retn
	}
}

// more functions
typedef void* (__cdecl * LoadModdableRawfile_t)(int a1, const char* filename);
LoadModdableRawfile_t LoadModdableRawfile = (LoadModdableRawfile_t)0x4BE4F0;

// wrapper function for LoadModdableRawfile
void* __cdecl LoadModdableRawfileFunc(const char* filename, void* buffer, int bufSize) {
	// call LoadModdableRawfile
	// we should actually return buffer instead, but as we can't make LoadModdableRawfile load to a custom buffer
	// and code shouldn't use the 'buffer' parameter directly, this should work in most cases
	// even then, we can hardly properly memcpy to the buffer as we don't know the actual file buffer size
	return LoadModdableRawfile(0, filename);
}

void __declspec(naked) LoadModdableRawfileStub() {
	__asm jmp LoadModdableRawfileFunc
}

// wrapper function for LoadModdableRawfile
void* __cdecl LoadModdableRawfileFunc2(const char* filename) {
	// call LoadModdableRawfile
	// we should actually return buffer instead, but as we can't make LoadModdableRawfile load to a custom buffer
	// and code shouldn't use the 'buffer' parameter directly, this should work in most cases
	// even then, we can hardly properly memcpy to the buffer as we don't know the actual file buffer size
	return LoadModdableRawfile(0, filename);
}

void __declspec(naked) LoadModdableRawfileStub2() {
	__asm jmp LoadModdableRawfileFunc2
}

CallHook rawFileHook1;
DWORD rawFileHook1Loc = 0x6332F5;

CallHook rawFileHook2;
DWORD rawFileHook2Loc = 0x5FCFDC;

CallHook rawFileHook3;
DWORD rawFileHook3Loc = 0x5FD046;

CallHook rawFileHook4;
DWORD rawFileHook4Loc = 0x63338F;

// TESTING FOR IRC INTEGRATION
typedef struct gentity_s {
	unsigned char pad[628];
} gentity_t;

gentity_t* entities = (gentity_t*)0x18DAF58;
int* maxclients = (int*)0x1ADAECC;

void G_SayTo(int mode, void* targetEntity, void* sourceEntity, int unknown, DWORD color, const char* name, const char* text) {
	DWORD func = 0x5E2320;

	__asm {
		push text
		push name
		push color
		push unknown
		push sourceEntity
		mov ecx, targetEntity
		mov eax, mode
		call func
		add esp, 14h
	}
}

void G_SayToAll(DWORD color, const char* name, const char* text) {
	gentity_t* sourceEntity = &entities[0];
	int unknown = 55;
	int mode = 0;

	for (int i = 0; i < *maxclients; i++) {
		gentity_t* other = &entities[i];

		G_SayTo(mode, other, sourceEntity, unknown, color, name, text);
	}
}

CallHook gRunFrameHook;
DWORD gRunFrameHookLoc = 0x4C6810;//0x62858D;
DWORD* clientState = (DWORD*)0xB32630;

void UI_DoServerRefresh(void);
void AC_PreRun();

DWORD lastACRun = 0;

void G_RunFrameHookFunc() {
	if (*clientState >= 9)
	{
		//RunFrameHook();
	}

	DWORD time = GetTickCount();
	if ((time - lastACRun) > 30000)
	{
		lastACRun = time;
		AC_PreRun();
	}

	UI_DoServerRefresh();
}

void __declspec(naked) G_RunFrameHookStub() {
	G_RunFrameHookFunc();

	__asm {
		jmp gRunFrameHook.pOriginal
	}
}

StompHook gSayHook;
DWORD gSayHookLoc = 0x41520D;

char* gSayName;
char* gSayText;

void G_SayHookFunc() {
	//SayHook(gSayName, gSayText);
}

void __declspec(naked) G_SayHookStub() {
	__asm mov eax, esp// + 4
	__asm add eax, 4
	__asm mov gSayName, eax

	__asm mov eax, esp// + 44h
	__asm add eax, 44h
	__asm mov gSayText, eax

	G_SayHookFunc();

	__asm add esp, 0DCh
	__asm retn
}

void GUI_Init();

BYTE newEnts[516 * 2048];

void ReallocTitties() {
	// allocate new memory
	int newsize = 1032 * 1024;//516 * 2048;
	//newEnts = malloc(newsize);

	// get (obfuscated) memory locations
	unsigned int origMin = 0x42B7B5 ^ 0xCDCDCD; // 8F7A78
	unsigned int origMax = 0x9D6E6E ^ 0x121212; // 8F7C7C

	unsigned int difference = (unsigned int)newEnts - origMin;

	// scan the .text memory
	char* scanMin = (char*)0x401000;
	char* scanMax = (char*)0x6D5B8A;
	char* current = scanMin;

	for (; current < scanMax; current += 1) {
		unsigned int* intCur = (unsigned int*)current;

		// if the address points to something within our range of interest
		if (*intCur >= origMin && *intCur <= origMax) {
			// patch it
			*intCur += difference;
		}
	}

	// give them a message they'll never forget
	//const char* message = "HELLO, DEAR <<CHEAT DEVELOPER NAME HERE>>. THIS MEMORY LOCATION WAS ONCE FILLED WITH 1 MB OF GOODNESS FOR YOU, BUT NOW IT'S EMPTY. WHY, YOU'D SAY? WELL, I'LL GIVE YOU A HINT: IT'S LOCKED AWAY SAFELY UNTIL THE CODE NEEDS THIS MEMORY DATA. WHEN THAT EXECUTES, YOU CAN HOOK IN, AND READ ALL YOU WANT OF THIS LOVELY DATA SECTION. ALSO, I WANT YOU TO POST A PICTURE OF YOUR LITTLE, CHILDISH FACE ONCE YOU NOTICE THIS MESSAGE (MOST OF YOU ARE LI'L KIDS, RIGHT?) TOGETHER WITH THIS MESSAGE YOURSELF ON <<CHEATING SITE NAME HERE>>. DON'T WORRY, I'LL GOOGLE IT. ALSO, AS SOME OF YOU HAVE FAILED TO KEEP THE WHITE FLAG UP FOR TOO LONG, I THINK IT'S SAFE TO SAY THE ARMS RACE CONTINUES. GOOD LUCK HACKING, OR IF YOU'RE SMART, YOU MOVE ON TO ANOTHER GAME. -- NTA ;)";
	//strcpy((char*)origMin, message);

	// and that should be it?
}

static char *rd_buffer;
static int rd_buffersize;
static void ( *rd_flush )( char *buffer );

void Com_BeginRedirect( char *buffer, int buffersize, void ( *flush )( char *) ) {
	if ( !buffer || !buffersize || !flush ) {
		return;
	}
	rd_buffer = buffer;
	rd_buffersize = buffersize;
	rd_flush = flush;

	*rd_buffer = 0;
}

void Com_EndRedirect( void ) {
	if ( rd_flush ) {
		rd_flush( rd_buffer );
	}

	rd_buffer = NULL;
	rd_buffersize = 0;
	rd_flush = NULL;
}

// com_printf to com_print
CallHook printfHook;
DWORD printfHookLoc = 0x45DB00;

// 'local' vars
int level;
const char* msg;
int wtf;

void __declspec(naked) PrintfHookStub()
{
	__asm push eax
	__asm mov eax, [esp + 8h]
	__asm mov level, eax
	__asm mov eax, [esp + 0Ch]
	__asm mov msg, eax
	__asm mov eax, [esp + 10h]
	__asm mov wtf, eax
	__asm pop eax

	if (rd_buffer)
	{
		// quick fix for some stack fuckups here
		__asm pushad
		if ( ( strlen( msg ) + strlen( rd_buffer ) ) > ( rd_buffersize - 1 ) ) {
			rd_flush( rd_buffer );
			*rd_buffer = 0;
		}
		strncat( rd_buffer, msg, rd_buffersize );

		__asm popad
		__asm retn
	}

	__asm jmp printfHook.pOriginal
}

/****************************
*	NTAuthority Code End	*
****************************/

typedef void (__cdecl * Cmd_ExecuteString_t)(int a1, int a2, char* cmd);
Cmd_ExecuteString_t Cmd_ExecuteString = (Cmd_ExecuteString_t)0x446DD0;

CallHook lobbyIDCheckHook;
DWORD lobbyIDCheckHookLoc = 0x4CA7D0;

void __declspec(naked) lobbyIDCheckHookStub() {

	__asm {
		mov eax, 1
		retn
	}
}

bool file_exists(char * fileName)
{
	// Check if a file exists
	struct stat buf;   
	int i = stat ( fileName, &buf );        
	if ( i == 0 )     
	{      
		return true;     
	}    
	return false;      
}

void ServerListInit();
//void statHandling();
void statsHook();
void lspHook();
void OoeyInit();
void StringLingInit();

void AIWPatch()
{
	// Delete logfile if it exists
	if(file_exists("steamAPI.log"))
	{
		remove("steamAPI.log");
	}

	Logger(lINFO, "alterIWnet", "Initializing client");

	// Remove protection
	DWORD oldProtect;
	VirtualProtect((void*)0x401000, 0x400000, PAGE_EXECUTE_READWRITE, &oldProtect);

	gamestateWriteHook.initialize("aaaaa", (PBYTE)gamestateWriteHookLoc);
	gamestateWriteHook.installHook(GamestateWriteHookStub, false);

	recordGamestate.initialize("aaaaa", (PBYTE)recordGamestateLoc);
	recordGamestate.installHook(RecordGamestateFunc, false);

	baselineStoreHook.initialize("aaaaa", 5, (PBYTE)baselineStoreHookLoc);
	baselineStoreHook.installHook(BaselineStoreHookFunc, true, false);

	baselineToFile.initialize("aaaaa", 5, (PBYTE)baselineToFileLoc);
	baselineToFile.installHook(BaselineToFileFunc, true, false);

	// testing for G_RunFrame stuff
	gSayHook.initialize("aaaaa", 5, (PBYTE)gSayHookLoc);
	gSayHook.installHook(G_SayHookStub, true, false);

	gRunFrameHook.initialize("aaaaa", (PBYTE)gRunFrameHookLoc);
	gRunFrameHook.installHook(G_RunFrameHookStub, false);

	// Rawfile moddability
	rawFileHook1.initialize("aaaaa", (PBYTE)rawFileHook1Loc);
	rawFileHook1.installHook(LoadModdableRawfileStub, false);

	rawFileHook2.initialize("aaaaa", (PBYTE)rawFileHook2Loc);
	rawFileHook2.installHook(LoadModdableRawfileStub, false);

	rawFileHook3.initialize("aaaaa", (PBYTE)rawFileHook3Loc);
	rawFileHook3.installHook(LoadModdableRawfileStub, false);

	rawFileHook4.initialize("aaaaa", (PBYTE)rawFileHook4Loc);
	rawFileHook4.installHook(LoadModdableRawfileStub2, false);

	//MENUtranslate_init();

	//*(BYTE*)0x5AFB2F = 0x90;
	//*(BYTE*)0x5AFB30 = 0xE9;
	*(BYTE*)0x60D515 = 0;

	// DLC material loading
	uiLoadHook1.initialize("aaaaa", (PBYTE)uiLoadHook1Loc);
	uiLoadHook1.installHook(UILoadHook1Stub, false);

	ffLoadHook1.initialize("aaaaa", (PBYTE)ffLoadHook1Loc);
	ffLoadHook1.installHook(FFLoadHook1Stub, false);

	ffLoadHook2.initialize("aaaaa", (PBYTE)ffLoadHook2Loc);
	ffLoadHook2.installHook(FFLoadHook2Stub, false);

	zoneLoadHook1.initialize("aaaaa", (PBYTE)zoneLoadHook1Loc);
	zoneLoadHook1.installHook(ZoneLoadHook1Stub, false);

	zoneLoadHook2.initialize("aaaaa", (PBYTE)zoneLoadHook2Loc);
	zoneLoadHook2.installHook(ZoneLoadHook2Stub, false);

	zoneLoadHook3.initialize("aaaaa", (PBYTE)zoneLoadHook3Loc);
	zoneLoadHook3.installHook(ZoneLoadHook2Stub, false);

	oobHandlerHook.initialize("aaaaa", (PBYTE)oobHandlerHookLoc);
	oobHandlerHook.installHook(OobHandlerHookStub, false);

	clientCommandsHook.initialize("aaaaa", (PBYTE)clientCommandsHookLoc);
	clientCommandsHook.installHook(ClientCommandsHookStub, false);

	execIsFSHook.initialize("aaaaa", (PBYTE)execIsFSHookLoc);
	execIsFSHook.installHook(ExecIsFSHookStub, false);

	//winMainInitHook.initialize("antic", (PBYTE)winMainInitHookLoc);
	//winMainInitHook.installHook(WinMainInitHookStub, false);

	clientIPUpdateHook.initialize("elite", (PBYTE)clientIPUpdateHookLoc);
	clientIPUpdateHook.installHook(ClientIPUpdateHookStub, false);

	// Lobby ID Validation bypass -No
	lobbyIDCheckHook.initialize("aaaaa", (PBYTE)lobbyIDCheckHookLoc);
	lobbyIDCheckHook.installHook(lobbyIDCheckHookStub, false);

	// Hook for console output redirection -No
	printfHook.initialize("compr", (PBYTE)printfHookLoc);
	printfHook.installHook(PrintfHookStub, false);

	ServerListInit();

	*(WORD*)0x586B6D = 0x9090;
	*(DWORD*)0x586BAD = (DWORD)hostConnection;
	*(DWORD*)0x586B93 = (DWORD)hIP;

	// Window titles
	//strcpy((char*)0x7859C0, "alterIWnet 2.0");
	strcpy((char*)0x7859C0, "alterIWnet 2.0 (DEV)");
	strcpy((char*)0x6EC9C4, "aIW Console");

	// Bottom-right corner message
	//*(DWORD*)0x5037EB = (DWORD)"alterIWnet ^32.0-BETA\n^7build ^1XX";
	*(DWORD*)0x5037EB = (DWORD)"alterIWnet ^32.0-DEV\n^7^1devbuild";

	*(BYTE*)0x4C5E7B = 0xEB;
	*(BYTE*)0x465107 = 0xEB;

	// test for iwd purification
	//*(BYTE*)0x747144 = 0;
	//*(BYTE*)0x47800B = 0xEB;
	memset((void*)0x45513D, 0x90, 5);
	memset((void*)0x45515B, 0x90, 5);
	memset((void*)0x45516C, 0x90, 5);
	
	memset((void*)0x45518E, 0x90, 5);
	memset((void*)0x45519F, 0x90, 5);

	*(BYTE*)0x449089 = 0xEB;

	// kill STEAMSTART check
	memset((void*)0x470A25, 0x90, 5);
	*(BYTE*)0x470A2C = 0xEB;

	*(DWORD*)0x4BBABB = (DWORD)"CoD6Host";

	// unhappy ping
	*(int*)0x4BBAE5 = 300;

	// badhost_endgameifisuck cheat protected
	*(BYTE*)0x4BBAF9 = 4;

	// remove tempBanClient
	//memset((void*)0x430590, 0x90, 5);
	//memset((void*)0x4305A4, 0x90, 5);

	// patch '1.3.37' version
	*(BYTE*)0x6FC8E6 = '3';

	const char* buildNumber = "37a++";
	strcpy((char*)0x73C4C0, buildNumber);

	// Client version 1.48
	*(int*)0x4024B1 = 50;

	// un-neuter Com_ParseCommandLine to allow non-connect_lobby
	*(BYTE*)0x4AECA4 = 0xEB;

	// protocol version (workaround for hacks)
	*(int*)0x41FB31 = 0x90;

	// function checking party heartbeat timeouts, causes random issues
	// memset((void*)0x4CAA5D, 0x90, 5);          not in latest !!!

	// protocol command
	*(int*)0x4BB9D9 = 0x90;
	*(int*)0x4BB9DE = 0x90;
	*(int*)0x4BB9E3 = 0x90;

	// Gamename
	const char* gamename = "aIW";
	*(DWORD*)0x5E6271 = (DWORD)gamename;

	// Build Date
	const char* buildDate = "20140701";
	strcpy((char*)0x6F1BB0, buildDate);

	// Stat file names
	//const char* statFile = "%s_aIW.stat";
	//strcpy((char*)0x748D6C, statFile);

	// max_dropped_weapons command
	*(BYTE*)0x5E640F = 0;

	// sv_network_fps max 1000, and uncheat
	*(BYTE*)0x4BBF79 = 0;
	*(DWORD*)0x4BBF7B = 1000;

	// remove fs_game stats
	*(WORD*)0x48D9C4 = 0x9090;

	// make statWriteNeeded ignored
	//*(BYTE*)0x424BD0 = 0xEB;

	// remove 'impure stats' notification
	*(BYTE*)0x501A30 = 0x33;
	*(BYTE*)0x501A31 = 0xC0;
	*(DWORD*)0x501A32 = 0xC3909090;

	// Patch server addresses
	/*
	strcpy((char*)0x7138BC, "192.168.1.3"); //team1.pc.iw4.iwnet.infinityward.com 
    strcpy((char*)0x7200B4, "192.168.1.3"); //web1.pc.iw4.iwnet.infinityward.com 
    strcpy((char*)0x72A900, "192.168.1.3"); //ip1.pc.iw4.iwnet.infinityward.com 
    strcpy((char*)0x734810, "192.168.1.3"); //log1.pc.iw4.iwnet.infinityward.com 
    strcpy((char*)0x6D781C, "192.168.1.3"); //match1.pc.iw4.iwnet.infinityward.com 
    strcpy((char*)0x6DA430, "192.168.1.3"); //auth1.pc.iw4.iwnet.infinityward.com 
    strcpy((char*)0x6E6358, "192.168.1.3"); //blob1.pc.iw4.iwnet.infinityward.com
	*/

	// Patch more addresses
	*(DWORD*)0x4CD220 = (DWORD)"http://192.168.1.3:13000/pc/";
	*(DWORD*)0x4CD23F = (DWORD)"http://192.168.1.3:13000/pc/";

	// de-cheat cg_fov
	*(BYTE*)0x4E1115 = 0;//40;

	// fs_game crash fix: disable some useless stuff
	*(BYTE*)0x481F1D = 0xEB;	
	*(WORD*)0x5BD502 = 0x9090;
	*(WORD*)0x5BD4BC = 0x9090;
	*(WORD*)0x5BDD02 = 0x9090;
	*(WORD*)0x5BDCBC = 0x9090;
	*(WORD*)0x5BD343 = 0x9090;
	*(WORD*)0x5BD354 = 0x9090;
	*(WORD*)0x5BDB43 = 0x9090;
	*(WORD*)0x5BDB54 = 0x9090;

	// .ff filename hash check (?)
	*(BYTE*)0x5BD4B2 = 0xB8;
	*(DWORD*)0x5BD4B3 = 1;

	*(BYTE*)0x5BD4F8 = 0xB8;
	*(DWORD*)0x5BD4F9 = 1;

	// remove limit on IWD file loading
	memset((void*)0x643B94, 0x90, 6);

	// remove convar write protection
	*(BYTE*)0x647DD4 = 0xEB;

	// remove fs_game check for moddable rawfiles
	*(WORD*)0x61C6D6 = 0x9090;

	// kill filesystem init default_mp.cfg check -- IW made it useless while moving .cfg files to fastfiles
	// and it makes fs_game crash

	// not nopping everything at once, there's cdecl stack cleanup in there
	memset((void*)0x4CDDFE, 0x90, 5);
	memset((void*)0x4CDE0A, 0x90, 5);
	memset((void*)0x4CDE12, 0x90, 0xB1);

	// for some reason fs_game != '' makes the game load mp_defaultweapon, which does not exist in MW2 anymore as a real asset
	// kill the call and make it act like fs_game == ''
	// UPDATE 2010-09-12: this is why CoD4 had text weapon files, those are used with fs_game.
	*(BYTE*)0x43A3ED = 0xEB;

	//*(DWORD*)0x44BE97 = (DWORD)leanLeftPTest;
	//*(DWORD*)0x44BEB0 = (DWORD)leanLeftMTest;

	// hopefully allow alt-tab during game, used at least in alt-enter handling
	*(BYTE*)0x4293D0 = 0xB0;
	*(BYTE*)0x4293D1 = 0x01;
	*(BYTE*)0x4293D2 = 0xC3;

	// increase maximum LSP packet size
	*(DWORD*)0x681E24 = 3500;
	*(DWORD*)0x681E3A = 3500;

	// cause Steam auth to always be requested
	memset((void*)0x46FB8C, 0x90, 6);

	// make newsfeed UI entries not cut based on safe area
	memset((void*)0x63982D, 0x90, 5);

	// HOTFIX: pingPlayer on dedicated servers
	*(BYTE*)0x48B4DA = 0xEB;

	// allow bind in all cases
	*(PBYTE)0x40C442 = (BYTE)0xEB;

	// 0x004caa5d is intact, disabled above patch

	// exec whitelist removal
	memset((void*)0x60BD95, 0x90, 5);
	*(WORD*)0x60BD9C = 0x9090;

	// ==== Missing hooks
	// 0040453a E9 C11FC00F              jmp 10006500
	// 00424960 E9 DB64BE0F              jmp 1000AE40
	// 004a48d0 E9 BB2CB60F              jmp 10007590

	// 00430fa4 E8 9798BD0F              call 1000A840  // LSP Stuff
	// 004356d1 E8 9A50BD0F              call 1000A770  // LSP Packet Sending
	// 00488a38 E8 23DBB70F              call 10006560  // UI_MP Stuff
	// 004aedb0 E8 FB6DB50F              call 10005BB0  // Crashrpt hook - Implemented but disabled
	// 004c6810 E8 DBFAB30F              call 100062F0  // AC_check, steam_id error !!
	// 004f266b E8 703FB10F              call 100065E0  // Say filter hook
	// 005aba7f E8 ECABA50F              call 10006670  // Q_vsnprintf
	// 005aba8d E8 6EAAA50F              call 10006500  // Rank related code
	// 0060cf3a E8 91D59F0F              call 1000A4D0  // Stat related code (Probably stat storage related?)
														// Check 0x60CCB0

	// ==== stat is now done in a few hooks, should be done in underneath global functions later
	// 00682600 E9 9B7C980F              jmp 1000A2A0    // read|write xxx_AIW.stat files
	// 00682820 E9 DB7B980F              jmp 1000A400    // read|write xxx_AIW.stat files

	//statHandling();
	statsHook();
	lspHook();
	OoeyInit();
	StringLingInit();
}