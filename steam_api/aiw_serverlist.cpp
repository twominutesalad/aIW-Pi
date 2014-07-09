/*
================================
Class that handles and feeds the
in-game server list
================================
*/

#include "stdafx.h"
#include "Hooking.h"
#include <WinSock2.h>

#pragma unmanaged
char	*Cmd_Argv( int arg );
int		Cmd_Argc( void );

typedef void (__cdecl * Cmd_ExecuteString_t)(int a1, int a2, char* cmd);
extern Cmd_ExecuteString_t Cmd_ExecuteString;

void Logger(unsigned int lvl, char* caller, char* logline, ...);

char* va( char *format, ... );

#define MAX_GLOBAL_SERVERS 2048

typedef bool qboolean;

// stuff for 'lan party' list
/*
#define SORT_HOST 0
#define SORT_MAP 1
#define SORT_CLIENTS 2
#define SORT_GAME 3
#define SORT_PING 4
*/

#define SORT_HOST 2
#define SORT_MAP 3
#define SORT_CLIENTS 4
#define SORT_GAME 5
#define SORT_HARDCORE 7
#define SORT_MOD 8
#define SORT_UPD 9
#define SORT_PING 10

typedef enum {
	NA_BOT,
	NA_BAD,					// an address lookup failed
	NA_LOOPBACK,
	NA_BROADCAST,
	NA_IP,
} netadrtype_t;

typedef enum {
	NS_CLIENT,
	NS_SERVER
} netsrc_t;

typedef struct {
	netadrtype_t	type;

	BYTE	ip[4];

	unsigned short	port;

	BYTE	ipx[10];
} netadr_t;

const char	*NET_AdrToString (netadr_t a);

typedef struct {
	netadr_t	adr;
	int			start;
	int			time;
	char		info[8192];
} ping_t;

ping_t	cl_pinglist[32];

typedef struct {
	netadr_t	adr;
	int			visible;
	char	  	hostName[1024];
	char	  	mapName[1024];
	char	  	game[1024];
	int			netType;
	char		gameType[1024];
	char		mod[1024];
	char		version[128];
	int			hardcore;
	int		  	clients;
	int		  	maxClients;
	int			minPing;
	int			maxPing;
	int			ping;
	int			punkbuster;
} serverInfo_t;

typedef struct {
	BYTE	ip[4];
	unsigned short	port;
} serverAddress_t;

typedef struct {
	int			numglobalservers;
	serverInfo_t  globalServers[MAX_GLOBAL_SERVERS];
	// additional global servers
	int			numGlobalServerAddresses;
	serverAddress_t		globalServerAddresses[MAX_GLOBAL_SERVERS];

	int pingUpdateSource;		// source currently pinging or updating

	int masterNum;
} clientStatic_t;

clientStatic_t cls;

typedef struct {
	char	adrstr[16];
	int		start;
} pinglist_t;


typedef struct serverStatus_s {
	pinglist_t pingList[32];
	int		numqueriedservers;
	int		currentping;
	int		nextpingtime;
	int		maxservers;
	int		refreshtime;
	int		numServers;
	int		sortKey;
	int		sortDir;
	int		lastCount;
	qboolean refreshActive;
	int		currentServer;
	int		displayServers[512];
	int		numDisplayServers;
	int		numPlayersOnServers;
	int		nextDisplayRefresh;
	int		nextSortTime;
} serverStatus_t;

typedef struct  
{
	serverStatus_t serverStatus;
} uiInfo_t;

uiInfo_t uiInfo;

typedef struct msg_s
{
	int unk1;
	int unk2;
	char* data;
	int unk3;
	int unk4;
	int cursize;
} msg_t;

typedef void (__cdecl * Com_Printf_t)(int, const char*, ...);
extern Com_Printf_t Com_Printf;

StompHook uiFeederCountHook;
DWORD uiFeederCountHookLoc = 0x476A10;
DWORD uiFeederCountHookRet = 0x476A17;

StompHook uiFeederItemTextHook;
DWORD uiFeederItemTextHookLoc = 0x46FCB0;
DWORD uiFeederItemTextHookRet = 0x46FCB7;

float currentFeeder;

float fl737C28 = 4.0f;

int UIFeederCountHookFunc()
{
	if (currentFeeder == 2.0f)
	{
		//return 75;
		return uiInfo.serverStatus.numDisplayServers;
	}

	return 0;
}


void __declspec(naked) UIFeederCountHookStub()
{
	__asm 
	{
		mov eax, [esp + 8h]
		mov currentFeeder, eax
		call UIFeederCountHookFunc
		test eax, eax
		jz continueRunning
		retn

continueRunning:
		push ecx
		fld fl737C28
		jmp uiFeederCountHookRet;
	}
		
}

int index;
int column;

char sting[256];

char* UIFeederItemTextHookFunc();

void __declspec(naked) UIFeederItemTextHookStub()
{
	__asm 
	{
		mov eax, [esp + 0Ch]
		mov currentFeeder, eax
		mov eax, [esp + 10h]
		mov index, eax
		mov eax, [esp + 14h]
		mov column, eax
		call UIFeederItemTextHookFunc
		test eax, eax
		jz continueRunning
		push ebx
		mov ebx, [esp + 4 + 28h]
		mov dword ptr [ebx], 0
		pop ebx
		retn

continueRunning:
		push ecx
		fld fl737C28
		jmp uiFeederItemTextHookRet
	}

}

StompHook uiFeederSelectionHook;
DWORD uiFeederSelectionHookLoc = 0x497040;
DWORD uiFeederSelectionHookRet = 0x497046;

bool UIFeederSelectionHookFunc()
{
	if (currentFeeder == 2.0f) {
		uiInfo.serverStatus.currentServer = index;

		return true;
	}

	return false;
}

void __declspec(naked) UIFeederSelectionHookStub()
{
	__asm 
	{
		mov eax, [esp + 08h]
		mov currentFeeder, eax
		mov eax, [esp + 0Ch]
		mov index, eax
		call UIFeederSelectionHookFunc
		fld fl737C28
		jmp uiFeederSelectionHookRet
	}

}

void CL_ServersResponsePacket( msg_t *msg );
void CL_ServerInfoPacket( netadr_t from, msg_t *msg );

bool wasGetServers;

void __declspec(naked) CL_ServersResponsePacketStub()
{
	__asm
	{
		push ebp //C54
		mov al, wasGetServers
		test al, al
		jz infoResponse
		// wasGetServers == 1
		call CL_ServersResponsePacket
		jmp endKey
infoResponse:
		// esp = C54h?
		mov eax, [esp + 0C54h + 14h]
		push eax
		mov eax, [esp + 0C58h + 10h]
		push eax
		mov eax, [esp + 0C5Ch + 0Ch]
		push eax
		mov eax, [esp + 0C60h + 08h]
		push eax
		mov eax, [esp + 0C64h + 04h]
		push eax
		call CL_ServerInfoPacket
		add esp, 14h
endKey:
		add esp, 4h
		mov al, 1
		        //C50
		pop edi //C4C
		pop esi //C48
		pop ebp //C44
		pop ebx //C40
		add esp, 0C40h
		retn
	}
}

CallHook gsrCmpHook;
DWORD gsrCmpHookLoc = 0x5AE119;

int GsrCmpHookFunc(const char* a1, const char* a2)
{
	bool result = _strnicmp(a1, a2, 18);

	if (!result)
	{
		wasGetServers = true;
	}
	else
	{
		wasGetServers = false;

		result = stricmp(a1, "infoResponse");
	}

	return (result);
}

void __declspec(naked) GsrCmpHookStub()
{
	__asm jmp GsrCmpHookFunc
}

char* uiS_name;
char* uiS_args;
DWORD uiS_continue = 0x49BC30;

void UI_StartServerRefresh(bool);
void UI_BuildServerDisplayList(bool);
void UI_ServersSort(int column, qboolean force);

typedef char* (__cdecl * COM_ParseExt_t)(char*);
COM_ParseExt_t COM_ParseExt = (COM_ParseExt_t)0x428F60;

bool Int_Parse(char* p, int* i)
{
	char* token;
	token = COM_ParseExt(p);

	if (token && token[0] != 0)
	{
		*i = atoi(token);
		return true;
	}

	return false;
}

static void LAN_GetServerAddressString( int source, int n, char *buf, int buflen ) {
	if (n >= 0 && n < MAX_GLOBAL_SERVERS) {
		strncpy(buf, NET_AdrToString( cls.globalServers[n].adr) , buflen );
		return;
	}

	buf[0] = '\0';
}

extern char SERVERipstring[256];
// returns true if handled, and false if not
bool UI_RunMenuScriptFunc()
{
	Logger(lINFO,"CIserverlist","Runmenu script [%s]",uiS_name);
	if (!stricmp(uiS_name, "RefreshServers"))
	{
		UI_StartServerRefresh(true);
		UI_BuildServerDisplayList(true);
		return true;
	}

	if (!stricmp(uiS_name, "RefreshFilter"))
	{
		UI_StartServerRefresh(false);
		UI_BuildServerDisplayList(true);
		return true;
	}

	if (!stricmp(uiS_name, "UpdateFilter"))
	{
		//UI_StartServerRefresh(true);
		UI_BuildServerDisplayList(true);
		//UI_FeederSelection(FEEDER_SERVERS, 0);
		return true;
	}

	if (!strcmp(uiS_name, "ServerSort"))
	{
		int sortColumn;
		if (Int_Parse(uiS_args, &sortColumn)) {
			// if same column we're already sorting on then flip the direction
			if (sortColumn == uiInfo.serverStatus.sortKey) {
				uiInfo.serverStatus.sortDir = !uiInfo.serverStatus.sortDir;
			}
			// make sure we sort again
			UI_ServersSort(sortColumn, true);
		}

		return true;
	}

	if (!strcmp(uiS_name, "JoinServer"))
	{
		char buff[1024];

		if (uiInfo.serverStatus.currentServer >= 0 && uiInfo.serverStatus.currentServer < uiInfo.serverStatus.numDisplayServers) {
			LAN_GetServerAddressString(0, uiInfo.serverStatus.displayServers[uiInfo.serverStatus.currentServer], buff, 1024);
	  	    Logger(lINFO,"CLIserverlist","connect [%d] &s",uiInfo.serverStatus.currentServer,buff);
			strcpy(SERVERipstring,buff);
			Cmd_ExecuteString( 0, 0, "gotoserver\n");
//			Cmd_ExecuteString( 0, 0, va( "connect %s\n", buff ) );
		}
	}
	return false;
}

void __declspec(naked) UI_RunMenuScriptStub()
{
	__asm
	{
		mov eax, esp
		add eax, 8h
		mov uiS_name, eax
		mov eax, [esp + 0C10h]
		mov uiS_args, eax
		call UI_RunMenuScriptFunc
		test eax, eax
		jz continue_uis

		// if returned
		pop edi
		pop esi
		add esp, 0C00h
		retn

continue_uis:
		jmp uiS_continue
	}
}

void ServerListInit()
{
	uiFeederCountHook.initialize("AaaAa", 7, (PBYTE)uiFeederCountHookLoc);
	uiFeederCountHook.installHook(UIFeederCountHookStub, true, false);

	uiFeederItemTextHook.initialize("AaaAa", 7, (PBYTE)uiFeederItemTextHookLoc);
	uiFeederItemTextHook.installHook(UIFeederItemTextHookStub, true, false);

	uiFeederSelectionHook.initialize("CAKE", 6, (PBYTE)uiFeederSelectionHookLoc);
	uiFeederSelectionHook.installHook(UIFeederSelectionHookStub, true, false);

	// getserversResponse
	*(DWORD*)0x5AE114 = (DWORD)"getserversResponse";

	*(int*)0x5AE125 = ((DWORD)CL_ServersResponsePacketStub) - 0x5AE123 - 6;

	// ui script stuff
	*(int*)0x49BB8B = ((DWORD)UI_RunMenuScriptStub) - 0x49BB89 - 6;

	gsrCmpHook.initialize("aaaaa", (PBYTE)gsrCmpHookLoc);
	gsrCmpHook.installHook(GsrCmpHookStub, false);

	// some thing overwriting feeder 2's data
	*(BYTE*)0x4BF369 = 0xEB;

	// some moar
	uiInfo.serverStatus.sortKey = SORT_PING;
	uiInfo.serverStatus.sortDir = false;
}

typedef bool (__cdecl * NET_StringToAdr_t)(const char*, netadr_t*);
NET_StringToAdr_t NET_StringToAdr = (NET_StringToAdr_t)0x48FEC0;

typedef void (__cdecl* sendOOB_t)(int, int, int, int, int, int, const char*);
sendOOB_t OOBPrint = (sendOOB_t)0x4A2170;

qboolean	NET_CompareAdr (netadr_t a, netadr_t b)
{
	if (a.type != b.type)
		return false;

	if (a.type == NA_LOOPBACK)
		return true;

	if (a.type == NA_IP)
	{
		if (a.ip[0] == b.ip[0] && a.ip[1] == b.ip[1] && a.ip[2] == b.ip[2] && a.ip[3] == b.ip[3] && a.port == b.port)
			return true;
		return false;
	}

	Com_Printf (0, "NET_CompareAdr: bad address type\n");
	return false;
}

int	NET_toipport(char *ipstr, int *port)
{
	char      *ptr;
	netadr_t  to;
	int		  ip;

	ip=0;
	*port=28960;
	ptr=strchr(ipstr,':');
	if (ptr) {
		*ptr='\0';
		*port = atoi(&ptr[1]);
		if (*port <= 0 || *port >= 65536) return 0;
	}
	memset( &to, 0, sizeof(netadr_t) );
	if ( NET_StringToAdr( ipstr, &to ) && to.type==NA_IP ) {
		memcpy((void *)&ip,(void *)&to.ip[0],4);
	}
	return ip;
}

void OOBPrintT(int type, netadr_t netadr, const char* message)
{
	int* adr = (int*)&netadr;

	OOBPrint(type, *adr, *(adr + 1), *(adr + 2), 0xFFFFFFFF, *(adr + 4), message);
}

void NET_OutOfBandPrint(int type, netadr_t adr, const char* message, ...)
{
	va_list args;
	char buffer[1024];

	va_start(args, message);
	_vsnprintf(buffer, sizeof(buffer), message, args);
	va_end(args);

	OOBPrintT(type, adr, buffer);
}

void CL_GlobalServers_f( void ) {
	netadr_t	to;
	int			i;
	int			count;
	char		*buffptr;
	char		command[1024];

	if ( Cmd_Argc() < 3) {
		Com_Printf( 0, "usage: globalservers <master# 0-1> <protocol> [keywords]\n");
		return;	
	}

	cls.masterNum = atoi( Cmd_Argv(1) );

	Com_Printf( 0, "Requesting servers from the master...\n");

	// reset the list, waiting for response
	// -1 is used to distinguish a "no response"

	NET_StringToAdr( /*"192.168.1.3", &to );//*/"192.168.1.3", &to );
	cls.numglobalservers = -1;
	cls.pingUpdateSource = 0;
	to.type = NA_IP;
	to.port = htons(20810);

	sprintf( command, "getservers IW4 %s", Cmd_Argv(2) );

	// tack on keywords
	buffptr = command + strlen( command );
	count   = Cmd_Argc();
	for (i=3; i<count; i++)
		buffptr += sprintf( buffptr, " %s", Cmd_Argv(i) );

	NET_OutOfBandPrint( NS_SERVER, to, command );
}

static void CL_SetServerInfoByAddress(netadr_t from, const char *info, int ping);

ping_t* CL_GetFreePing( void )
{
	ping_t*	pingptr;
	ping_t*	best;	
	int		oldest;
	int		i;
	int		time;

	pingptr = cl_pinglist;
	for (i=0; i<32; i++, pingptr++ )
	{
		// find free ping slot
		if (pingptr->adr.port)
		{
			if (!pingptr->time)
			{
				if (GetTickCount() - pingptr->start < 500)
				{
					// still waiting for response
					continue;
				}
			}
			else if (pingptr->time < 500)
			{
				// results have not been queried
				continue;
			}
		}

		// clear it
		pingptr->adr.port = 0;
		return (pingptr);
	}

	// use oldest entry
	pingptr = cl_pinglist;
	best    = cl_pinglist;
	oldest  = INT_MIN;
	for (i=0; i<32; i++, pingptr++ )
	{
		// scan for oldest
		time = GetTickCount() - pingptr->start;
		if (time > oldest)
		{
			oldest = time;
			best   = pingptr;
		}
	}

	return (best);
}

void CL_Ping_f( void ) {
	netadr_t	to;
	ping_t*		pingptr;
	char*		server;

	if ( Cmd_Argc() != 2 ) {
		Com_Printf( 0, "usage: ping [server]\n");
		return;	
	}

	memset( &to, 0, sizeof(netadr_t) );

	server = Cmd_Argv(1);

	if ( !NET_StringToAdr( server, &to ) ) {
		return;
	}

	pingptr = CL_GetFreePing();

	memcpy( &pingptr->adr, &to, sizeof (netadr_t) );
	pingptr->start = GetTickCount();
	pingptr->time  = 0;

	CL_SetServerInfoByAddress(pingptr->adr, NULL, 0);

	NET_OutOfBandPrint( NS_CLIENT, to, "getinfo xxx" );
}

#define MAX_PINGREQUESTS 32

/*
==================
CL_GetPing
==================
*/
void CL_GetPing( int n, char *buf, int buflen, int *pingtime )
{
	const char	*str;
	int		time;
	int		maxPing;

	if (!cl_pinglist[n].adr.port)
	{
		// empty slot
		buf[0]    = '\0';
		*pingtime = 0;
		return;
	}

	str = NET_AdrToString( cl_pinglist[n].adr );
	strncpy( buf, str, buflen );

	time = cl_pinglist[n].time;
	if (!time)
	{
		// check for timeout
		time = GetTickCount() - cl_pinglist[n].start;
		//maxPing = Cvar_VariableIntegerValue( "cl_maxPing" );
		maxPing = 500;
		if (time < maxPing)
		{
			// not timed out yet
			time = 0;
		}
	}

	CL_SetServerInfoByAddress(cl_pinglist[n].adr, cl_pinglist[n].info, cl_pinglist[n].time);

	*pingtime = time;
}

/*
==================
CL_UpdateServerInfo
==================
*/
void CL_UpdateServerInfo( int n )
{
	if (!cl_pinglist[n].adr.port)
	{
		return;
	}

	//CL_SetServerInfoByAddress(cl_pinglist[n].adr, cl_pinglist[n].info, cl_pinglist[n].time );
}

/*
==================
CL_GetPingInfo
==================
*/
void CL_GetPingInfo( int n, char *buf, int buflen )
{
	if (!cl_pinglist[n].adr.port)
	{
		// empty slot
		if (buflen)
			buf[0] = '\0';
		return;
	}

	strncpy( buf, cl_pinglist[n].info, buflen );
}

/*
==================
CL_ClearPing
==================
*/
void CL_ClearPing( int n )
{
	if (n < 0 || n >= MAX_PINGREQUESTS)
		return;

	cl_pinglist[n].adr.port = 0;
}

/*
==================
CL_GetPingQueueCount
==================
*/
int CL_GetPingQueueCount( void )
{
	int		i;
	int		count;
	ping_t*	pingptr;

	count   = 0;
	pingptr = cl_pinglist;

	for (i=0; i<MAX_PINGREQUESTS; i++, pingptr++ ) {
		if (pingptr->adr.port) {
			count++;
		}
	}

	return (count);
}

/*
===================
CL_InitServerInfo
===================
*/
void CL_InitServerInfo( serverInfo_t *server, serverAddress_t *address ) {
	server->adr.type  = NA_IP;
	server->adr.ip[0] = address->ip[0];
	server->adr.ip[1] = address->ip[1];
	server->adr.ip[2] = address->ip[2];
	server->adr.ip[3] = address->ip[3];
	server->adr.port  = address->port;
	server->clients = 0;
	server->hostName[0] = '\0';
	server->mapName[0] = '\0';
	server->maxClients = 0;
	server->maxPing = 0;
	server->minPing = 0;
	server->ping = -1;
	server->game[0] = '\0';
	server->gameType[0] = '\0';
	server->netType = 0;
}

qboolean CL_UpdateVisiblePings_f(int source) {
	int			slots, i;
	char		buff[8192];
	int			pingTime;
	int			max;
	qboolean status = false;

	cls.pingUpdateSource = source;

	slots = CL_GetPingQueueCount();
	if (slots < MAX_PINGREQUESTS) {
		serverInfo_t *server = NULL;

		server = &cls.globalServers[0];
		max = cls.numglobalservers;

		for (i = 0; i < max; i++) {
			if (server[i].visible) {
				if (server[i].adr.type != NA_IP)
				{
					continue;;
				}

				if (server[i].ping == -1) {
					int j;

					if (slots >= MAX_PINGREQUESTS) {
						break;
					}
					for (j = 0; j < MAX_PINGREQUESTS; j++) {
						if (!cl_pinglist[j].adr.port) {
							continue;
						}
						if (NET_CompareAdr( cl_pinglist[j].adr, server[i].adr)) {
							// already on the list
							break;
						}
					}
					if (j >= MAX_PINGREQUESTS) {
						status = true;
						for (j = 0; j < MAX_PINGREQUESTS; j++) {
							if (!cl_pinglist[j].adr.port) {
								break;
							}
						}
						memcpy(&cl_pinglist[j].adr, &server[i].adr, sizeof(netadr_t));
						cl_pinglist[j].start = GetTickCount();
						cl_pinglist[j].time = 0;
						NET_OutOfBandPrint( NS_CLIENT, cl_pinglist[j].adr, "getinfo xxx" );
						slots++;
					}
				}
				// if the server has a ping higher than cl_maxPing or
				// the ping packet got lost
				else if (server[i].ping == 0) {
					// if we are updating global servers
					if ( cls.numGlobalServerAddresses > 0 ) {
						// overwrite this server with one from the additional global servers
						cls.numGlobalServerAddresses--;
						CL_InitServerInfo(&server[i], &cls.globalServerAddresses[cls.numGlobalServerAddresses]);
						// NOTE: the server[i].visible flag stays untouched
					}
				}
			}
		}
	} 

	if (slots) {
		status = true;
	}
	for (i = 0; i < MAX_PINGREQUESTS; i++) {
		if (!cl_pinglist[i].adr.port) {
			continue;
		}
		CL_GetPing( i, buff, 8192, &pingTime );
		if (pingTime != 0) {
			CL_ClearPing(i);
			status = true;
		}
	}

	return status;
}

char *Info_ValueForKey( const char *s, const char *key );
void Info_SetValueForKey( char *s, const char *key, const char *value );

typedef void (__cdecl * Com_DPrintf_t)(int, const char*, ...);
Com_DPrintf_t Com_DPrintf = (Com_DPrintf_t)0x47CB30;

typedef char* (__cdecl * MSG_ReadString_t)(msg_t*);
MSG_ReadString_t MSG_ReadString = (MSG_ReadString_t)0x40EB10;

static void CL_SetServerInfo(serverInfo_t *server, const char *info, int ping) {
	if (server) {
		if (info) {
			//char* bigMC = Info_ValueForKey(info, "sv_maxclients");
			//int mcNugger = atoi(bigMC);

			server->clients = atoi(Info_ValueForKey(info, "clients"));
			strncpy(server->mod, Info_ValueForKey(info, "fs_game"), 1024);
			strncpy(server->version, Info_ValueForKey(info, "shortversion"), 128);
			strncpy(server->hostName,Info_ValueForKey(info, "hostname"), 1024);
			strncpy(server->mapName, Info_ValueForKey(info, "mapname"), 1024);
			server->maxClients = atoi(Info_ValueForKey(info, "sv_maxclients"));
			server->hardcore = atoi(Info_ValueForKey(info, "hc"));
			strncpy(server->game,Info_ValueForKey(info, "game"), 1024);
			//server->gameType = Info_ValueForKey(info, "gametype");
			strncpy(server->gameType, Info_ValueForKey(info, "gametype"), 1024);
			server->netType = atoi(Info_ValueForKey(info, "nettype"));
			server->minPing = atoi(Info_ValueForKey(info, "minping"));
			server->maxPing = atoi(Info_ValueForKey(info, "maxping"));
			//server->punkbuster = atoi(Info_ValueForKey(info, "punkbuster"));
		}
		server->ping = ping;

//		Com_DPrintf(0, "Setting ping to %d for %s\n", ping, NET_AdrToString(server->adr));
	}
}

static void CL_SetServerInfoByAddress(netadr_t from, const char *info, int ping) {
	int i;

	for (i = 0; i < MAX_GLOBAL_SERVERS; i++) {
		if (NET_CompareAdr(from, cls.globalServers[i].adr)) {
			CL_SetServerInfo(&cls.globalServers[i], info, ping);
		}
	}
}

void CL_ServerInfoPacket( netadr_t from, msg_t *msg )
{
	int		i, type;
	char	info[1024];
	char*	str;
	char	*infoString;
	int		prot;

	infoString = MSG_ReadString( msg );
	//infoString = msg->data;

	// if this isn't the correct protocol version, ignore it
	/*prot = atoi( Info_ValueForKey( infoString, "protocol" ) );
	if ( prot != 144 ) {
		Com_DPrintf( "Different protocol info packet: %s\n", infoString );
		return;
	}*/

	// iterate servers waiting for ping response
	for (i=0; i<MAX_PINGREQUESTS; i++)
	{
		if ( cl_pinglist[i].adr.port && !cl_pinglist[i].time && NET_CompareAdr( from, cl_pinglist[i].adr ) )
		{
			// calc ping time
			cl_pinglist[i].time = GetTickCount() - cl_pinglist[i].start + 1;
			Com_DPrintf( 0, "ping time %dms from %s\n", cl_pinglist[i].time, NET_AdrToString( from ) );

			// save of info
			strncpy( cl_pinglist[i].info, infoString, sizeof( cl_pinglist[i].info ) );

			// tack on the net type
			// NOTE: make sure these types are in sync with the netnames strings in the UI
			switch (from.type)
			{
			case NA_BROADCAST:
			case NA_IP:
				str = "udp";
				type = 1;
				break;

			default:
				str = "???";
				type = 0;
				break;
			}
			Info_SetValueForKey( cl_pinglist[i].info, "nettype", va("%d", type) );
			CL_SetServerInfoByAddress(from, infoString, cl_pinglist[i].time);

			return;
		}
	}
}

#define MAX_SERVERSPERPACKET	256

/*
===================
CL_ServersResponsePacket
===================
*/
void CL_ServersResponsePacket( msg_t *msg ) {
	int				i, count, max, total;
	serverAddress_t addresses[MAX_SERVERSPERPACKET];
	int				numservers;
	char*			buffptr;
	char*			buffend;
	
	Com_Printf(0, "CL_ServersResponsePacket\n");

	if (cls.numglobalservers == -1) {
		// state to detect lack of servers or lack of response
		cls.numglobalservers = 0;
		cls.numGlobalServerAddresses = 0;
	}

	// parse through server response string
	numservers = 0;
	buffptr    = msg->data;
	buffend    = buffptr + msg->cursize;
	while (buffptr+1 < buffend) {
		// advance to initial token
		do {
			if (*buffptr++ == '\\')
				break;		
		}
		while (buffptr < buffend);

		if ( buffptr >= buffend - 6 ) {
			break;
		}

		// parse out ip
		addresses[numservers].ip[0] = *buffptr++;
		addresses[numservers].ip[1] = *buffptr++;
		addresses[numservers].ip[2] = *buffptr++;
		addresses[numservers].ip[3] = *buffptr++;

		// parse out port
		addresses[numservers].port = (*(buffptr++))<<8;
		addresses[numservers].port += (*(buffptr++)) & 0xFF;
		addresses[numservers].port = ntohs( addresses[numservers].port );

		// syntax check
		if (*buffptr != '\\') {
			break;
		}

		Com_DPrintf( 0, "server: %d ip: %d.%d.%d.%d:%d\n",numservers,
				addresses[numservers].ip[0],
				addresses[numservers].ip[1],
				addresses[numservers].ip[2],
				addresses[numservers].ip[3],
				ntohs(addresses[numservers].port) );

		numservers++;
		if (numservers >= MAX_SERVERSPERPACKET) {
			break;
		}

		// parse out EOT
		if (buffptr[1] == 'E' && buffptr[2] == 'O' && buffptr[3] == 'T') {
			break;
		}
	}

	count = cls.numglobalservers;
	max = MAX_GLOBAL_SERVERS;

	for (i = 0; i < numservers && count < max; i++) {
		// build net address
		serverInfo_t *server = &cls.globalServers[count];

		CL_InitServerInfo( server, &addresses[i] );
		// advance to next slot
		count++;
	}

	// if getting the global list
	if (cls.masterNum == 0) {
		if ( cls.numGlobalServerAddresses < MAX_GLOBAL_SERVERS ) {
			// if we couldn't store the servers in the main list anymore
			for (; i < numservers && count >= max; i++) {
				serverAddress_t *addr;
				// just store the addresses in an additional list
				addr = &cls.globalServerAddresses[cls.numGlobalServerAddresses++];
				addr->ip[0] = addresses[i].ip[0];
				addr->ip[1] = addresses[i].ip[1];
				addr->ip[2] = addresses[i].ip[2];
				addr->ip[3] = addresses[i].ip[3];
				addr->port  = addresses[i].port;
			}
		}
	}

	cls.numglobalservers = count;
	total = count + cls.numGlobalServerAddresses;

	Com_Printf(0, "%d servers parsed (total %d)\n", numservers, total);
}

/*
====================
LAN_GetServerPtr
====================
*/
static serverInfo_t *LAN_GetServerPtr( int source, int n ) {
	if (n >= 0 && n < MAX_GLOBAL_SERVERS) {
		return &cls.globalServers[n];
	}
	return NULL;
}

struct mapArena_t
{
	char uiName[32];
	char mapName[16];
	char pad[2768];
};

typedef char* (__cdecl * LocalizeString_t)(char*, char*);
LocalizeString_t LocalizeString = (LocalizeString_t)0x4247D0;

typedef char* (__cdecl * LocalizeMapString_t)(char*);
LocalizeMapString_t LocalizeMapString = (LocalizeMapString_t)0x4A4380;

int* _arenaCount = (int*)0x633E430;
mapArena_t* _arenas = (mapArena_t*)0x633E434;

char* UI_LocalizeMapName(char* mapName)
{
	for (int i = 0; i < *_arenaCount; i++)
	{
		if (!_stricmp(_arenas[i].mapName, mapName))
		{
			char* name = LocalizeMapString(_arenas[i].uiName);
			return name;
		}
	}

	return mapName;
}

struct gameTypeName_t
{
	char gameType[12];
	char uiName[32];
};

int* _gtCount = (int*)0x633CBA0;
gameTypeName_t* _types = (gameTypeName_t*)0x633CBA4;

char* UI_LocalizeGameType(char* gameType)
{
	if (gameType == 0 || *gameType == '\0')
	{
		return "";
	}

	// workaround until localized
	if (_stricmp(gameType, "oitc") == 0)
	{
		return "One in the Chamber";
	}

	if (_stricmp(gameType, "gg") == 0)
	{
		return "Gun Game";
	}

	if (_stricmp(gameType, "ss") == 0)
	{
		return "Sharpshooter";
	}

	for (int i = 0; i < *_gtCount; i++)
	{
		if (!_stricmp(_types[i].gameType, gameType))
		{
			char* name = LocalizeMapString(_types[i].uiName);
			return name;
		}
	}

	return gameType;
}

/*
====================
LAN_CompareServers
====================
*/
static int LAN_CompareServers( int source, int sortKey, int sortDir, int s1, int s2 ) {
	int res;
	serverInfo_t *server1, *server2;
	char *gt1, *gt2;

	server1 = LAN_GetServerPtr(source, s1);
	server2 = LAN_GetServerPtr(source, s2);
	if (!server1 || !server2) {
		return 0;
	}

	res = 0;
	switch( sortKey ) {
		case SORT_HOST:
			res = _stricmp( server1->hostName, server2->hostName );
			break;

		case SORT_MAP:
			res = _stricmp( UI_LocalizeMapName(server1->mapName), UI_LocalizeMapName(server2->mapName) );
			break;
		case SORT_CLIENTS:
			if (server1->clients < server2->clients) {
				res = -1;
			}
			else if (server1->clients > server2->clients) {
				res = 1;
			}
			else {
				res = 0;
			}
			break;
		case SORT_GAME:
			gt1 = UI_LocalizeGameType(server1->gameType);
			gt2 = UI_LocalizeGameType(server2->gameType);

			if (gt1 && gt2)
				res = _stricmp( UI_LocalizeGameType(server1->gameType), UI_LocalizeGameType(server2->gameType) );
			break;
		case SORT_HARDCORE:
			if ((server1->hardcore & 0x1) < (server2->hardcore & 0x1)) {
				res = -1;
			}
			else if ((server1->hardcore & 0x1) > (server2->hardcore & 0x1)) {
				res = 1;
			}
			else {
				res = 0;
			}
			break;
		case SORT_MOD:
			res = _stricmp( server1->mod, server2->mod );
			break;
		case SORT_UPD:
			res = _stricmp( server1->version, server2->version );
			break;
		case SORT_PING:
			if (server1->ping < server2->ping) {
				res = -1;
			}
			else if (server1->ping > server2->ping) {
				res = 1;
			}
			else {
				res = 0;
			}
			break;
	}

	if (res < 0) res = -1;
	if (res > 0) res = 1;

	if (sortDir) {
		if (res < 0)
			return 1;
		if (res > 0)
			return -1;
		return 0;
	}
	return res;
}

static int LAN_GetServerCount( int source ) {
	return cls.numglobalservers;
}

static void LAN_MarkServerVisible(int source, int n, qboolean visible ) {
	if (n == -1) {
		int count;
		serverInfo_t *server = NULL;
		server = &cls.globalServers[0];
		count = MAX_GLOBAL_SERVERS;

		if (server) {
			for (n = 0; n < count; n++) {
				server[n].visible = visible;
			}
		}

	} else {
		if (n >= 0 && n < MAX_GLOBAL_SERVERS) {
			cls.globalServers[n].visible = visible;
		}
	}
}

static int LAN_GetServerPing( int source, int n ) {
	serverInfo_t *server = NULL;
	if (n >= 0 && n < MAX_GLOBAL_SERVERS) {
		server = &cls.globalServers[n];
	}

	if (server) {
		return server->ping;
	}
	return -1;
}

static int LAN_ServerIsVisible(int source, int n ) {
	if (n >= 0 && n < MAX_GLOBAL_SERVERS) {
		return cls.globalServers[n].visible;
	}

	return false;
}

const char	*NET_AdrToString (netadr_t a)
{
	static	char	s[64];

	if (a.type == NA_LOOPBACK) {
		_snprintf (s, sizeof(s), "loopback");
	} else if (a.type == NA_BOT) {
		_snprintf (s, sizeof(s), "bot");
	} else if (a.type == NA_IP) {
		_snprintf (s, sizeof(s), "%i.%i.%i.%i:%hu",
			a.ip[0], a.ip[1], a.ip[2], a.ip[3], ntohs(a.port));
	} else {
		_snprintf (s, sizeof(s), "%02x%02x%02x%02x.%02x%02x%02x%02x%02x%02x:%hu",
			a.ipx[0], a.ipx[1], a.ipx[2], a.ipx[3], a.ipx[4], a.ipx[5], a.ipx[6], a.ipx[7], a.ipx[8], a.ipx[9], 
			ntohs(a.port));
	}

	return s;
}

static void LAN_GetServerInfo( int source, int n, char *buf, int buflen ) {
	char info[8192];
	serverInfo_t *server = NULL;
	info[0] = '\0';
	if (n >= 0 && n < MAX_GLOBAL_SERVERS) {
		server = &cls.globalServers[n];
	}

	if (server && buf) {
		buf[0] = '\0';
		Info_SetValueForKey( info, "hostname", server->hostName);
		Info_SetValueForKey( info, "fs_game", server->mod);
		Info_SetValueForKey( info, "hc", va("%i", server->hardcore));
		Info_SetValueForKey( info, "shortversion", server->version);
		Info_SetValueForKey( info, "mapname", server->mapName);
		Info_SetValueForKey( info, "clients", va("%i",server->clients));
		Info_SetValueForKey( info, "sv_maxclients", va("%i",server->maxClients));
		Info_SetValueForKey( info, "ping", va("%i",server->ping));
		Info_SetValueForKey( info, "minping", va("%i",server->minPing));
		Info_SetValueForKey( info, "maxping", va("%i",server->maxPing));
		Info_SetValueForKey( info, "game", server->game);
		Info_SetValueForKey( info, "gametype", server->gameType);
		Info_SetValueForKey( info, "nettype", va("%i",server->netType));
		Info_SetValueForKey( info, "addr", NET_AdrToString(server->adr));
		//Info_SetValueForKey( info, "punkbuster", va("%i", server->punkbuster));
		strncpy(buf, info, buflen);
	} else {
		if (buf) {
			buf[0] = '\0';
		}
	}
}

qboolean LAN_UpdateVisiblePings(int source ) {
	return CL_UpdateVisiblePings_f(source);
}

#define	BIG_INFO_STRING		8192  // used for system info key only
#define	BIG_INFO_KEY		  8192
#define	BIG_INFO_VALUE		8192

/*
===============
Info_ValueForKey

Searches the string for the given
key and returns the associated value, or an empty string.
FIXME: overflow check?
===============
*/
char *Info_ValueForKey( const char *s, const char *key ) {
	char	pkey[BIG_INFO_KEY];
	static	char value[2][BIG_INFO_VALUE];	// use two buffers so compares
											// work without stomping on each other
	static	int	valueindex = 0;
	char	*o;
	
	if ( !s || !key ) {
		return "";
	}

	if ( strlen( s ) >= BIG_INFO_STRING ) {
		//Com_Error( ERR_DROP, "Info_ValueForKey: oversize infostring" );
		return "";
	}

	valueindex ^= 1;
	if (*s == '\\')
		s++;
	while (1)
	{
		o = pkey;
		while (*s != '\\')
		{
			if (!*s)
				return "";
			*o++ = *s++;
		}
		*o = 0;
		s++;

		o = value[valueindex];

		while (*s != '\\' && *s)
		{
			*o++ = *s++;
		}
		*o = 0;

		if (!_stricmp (key, pkey) )
			return value[valueindex];

		if (!*s)
			break;
		s++;
	}

	return "";
}


/*
=================
ArenaServers_StopRefresh
=================
*/
static void UI_StopServerRefresh( void )
{
	int count;

	if (!uiInfo.serverStatus.refreshActive) {
		// not currently refreshing
		return;
	}
	uiInfo.serverStatus.refreshActive = false;
	Com_Printf(0, "%d servers listed in browser with %d players.\n",
					uiInfo.serverStatus.numDisplayServers,
					uiInfo.serverStatus.numPlayersOnServers);
	count = LAN_GetServerCount(0);
	if (count - uiInfo.serverStatus.numDisplayServers > 0) {
		Com_Printf(0, "%d servers not listed due to packet loss or pings higher than %d\n",
						count - uiInfo.serverStatus.numDisplayServers,
						500);
	}

}

/*
==================
UI_InsertServerIntoDisplayList
==================
*/
static void UI_InsertServerIntoDisplayList(int num, int position) {
	int i;

	if (position < 0 || position > uiInfo.serverStatus.numDisplayServers ) {
		return;
	}
	//
	uiInfo.serverStatus.numDisplayServers++;
	for (i = uiInfo.serverStatus.numDisplayServers; i > position; i--) {
		uiInfo.serverStatus.displayServers[i] = uiInfo.serverStatus.displayServers[i-1];
	}
	uiInfo.serverStatus.displayServers[position] = num;
}

/*
==================
UI_RemoveServerFromDisplayList
==================
*/
static void UI_RemoveServerFromDisplayList(int num) {
	int i, j;

	for (i = 0; i < uiInfo.serverStatus.numDisplayServers; i++) {
		if (uiInfo.serverStatus.displayServers[i] == num) {
			uiInfo.serverStatus.numDisplayServers--;
			for (j = i; j < uiInfo.serverStatus.numDisplayServers; j++) {
				uiInfo.serverStatus.displayServers[j] = uiInfo.serverStatus.displayServers[j+1];
			}
			return;
		}
	}
}

/*
==================
UI_BinaryServerInsertion
==================
*/
static void UI_BinaryServerInsertion(int num) {
	int mid, offset, res, len;

	// use binary search to insert server
	len = uiInfo.serverStatus.numDisplayServers;
	mid = len;
	offset = 0;
	res = 0;
	while(mid > 0) {
		mid = len >> 1;
		//
		res = LAN_CompareServers( 0, uiInfo.serverStatus.sortKey,
					uiInfo.serverStatus.sortDir, num, uiInfo.serverStatus.displayServers[offset+mid]);
		// if equal
		if (res == 0) {
			UI_InsertServerIntoDisplayList(num, offset+mid);
			return;
		}
		// if larger
		else if (res == 1) {
			offset += mid;
			len -= mid;
		}
		// if smaller
		else {
			len -= mid;
		}
	}
	if (res == 1) {
		offset++;
	}
	UI_InsertServerIntoDisplayList(num, offset);
}

/*
==================
UI_BuildServerDisplayList
==================
*/
static void UI_BuildServerDisplayList(bool force) {
	int i, count, clients, maxClients, ping, game, len, visible;
	char info[4096];
//	qboolean startRefresh = qtrue; TTimo: unused
	static int numinvisible;

	if (!(force || GetTickCount() > uiInfo.serverStatus.nextDisplayRefresh)) {
		return;
	}
	// if we shouldn't reset
	if ( force == 2 ) {
		force = 0;
	}

	if (force) {
		numinvisible = 0;
		// clear number of displayed servers
		uiInfo.serverStatus.numDisplayServers = 0;
		uiInfo.serverStatus.numPlayersOnServers = 0;
		// set list box index to zero
		//Menu_SetFeederSelection(NULL, FEEDER_SERVERS, 0, NULL);
		// mark all servers as visible so we store ping updates for them
//		Com_DPrintf(0, "Marking servers as visible.\n");
		LAN_MarkServerVisible(0, -1, true);
	}

	// get the server count (comes from the master)
	count = LAN_GetServerCount(0);
	if (count == -1) {
		// still waiting on a response from the master
		uiInfo.serverStatus.numDisplayServers = 0;
		uiInfo.serverStatus.numPlayersOnServers = 0;
		uiInfo.serverStatus.nextDisplayRefresh = GetTickCount() + 500;
		return;
	}

	visible = false;
	for (i = 0; i < count; i++) {
		// if we already got info for this server
		if (!LAN_ServerIsVisible(0, i)) {
			continue;
		}
		visible = true;
		// get the ping for this server
		ping = LAN_GetServerPing(0, i);
//		Com_DPrintf(0, "Getting ping for server %d\n", i);
		if (ping > 0) {

			LAN_GetServerInfo(0, i, info, sizeof(info));

			clients = atoi(Info_ValueForKey(info, "clients"));
			uiInfo.serverStatus.numPlayersOnServers += clients;

			/*if (ui_browserShowEmpty.integer == 0) {
				if (clients == 0) {
					trap_LAN_MarkServerVisible(ui_netSource.integer, i, qfalse);
					continue;
				}
			}

			if (ui_browserShowFull.integer == 0) {
				maxClients = atoi(Info_ValueForKey(info, "sv_maxclients"));
				if (clients == maxClients) {
					trap_LAN_MarkServerVisible(ui_netSource.integer, i, qfalse);
					continue;
				}
			}

			if (uiInfo.joinGameTypes[ui_joinGameType.integer].gtEnum != -1) {
				game = atoi(Info_ValueForKey(info, "gametype"));
				if (game != uiInfo.joinGameTypes[ui_joinGameType.integer].gtEnum) {
					trap_LAN_MarkServerVisible(ui_netSource.integer, i, qfalse);
					continue;
				}
			}
				
			if (ui_serverFilterType.integer > 0) {
				if (Q_stricmp(Info_ValueForKey(info, "game"), serverFilters[ui_serverFilterType.integer].basedir) != 0) {
					trap_LAN_MarkServerVisible(ui_netSource.integer, i, qfalse);
					continue;
				}
			}
			// make sure we never add a favorite server twice
			if (ui_netSource.integer == AS_FAVORITES) {
				UI_RemoveServerFromDisplayList(i);
			}*/
			// insert the server into the list
			UI_BinaryServerInsertion(i);
			// done with this server
			if (ping > 0) {
				LAN_MarkServerVisible(0, i, false);
				numinvisible++;
			}
		}
	}

	uiInfo.serverStatus.refreshtime = GetTickCount();

	// if there were no servers visible for ping updates
	if (!visible) {
//		UI_StopServerRefresh();
//		uiInfo.serverStatus.nextDisplayRefresh = 0;
	}
}

/*
=================
UI_DoServerRefresh
=================
*/
void UI_DoServerRefresh( void )
{
	qboolean wait = false;

	if (!uiInfo.serverStatus.refreshActive) {
		return;
	}

	if (LAN_GetServerCount(0) < 0) {
		wait = true;
	}

	if (GetTickCount() < uiInfo.serverStatus.refreshtime) {
		if (wait) {
			return;
		}
	}

	// if still trying to retrieve pings
	if (LAN_UpdateVisiblePings(0)) {
		uiInfo.serverStatus.refreshtime = GetTickCount() + 1000;
	} else if (!wait) {
		// get the last servers in the list
		UI_BuildServerDisplayList(2);
		// stop the refresh
		UI_StopServerRefresh();
	}
	//
	UI_BuildServerDisplayList(false);
}

static void LAN_ResetPings(int source) {
	int count,i;
	serverInfo_t *servers = NULL;
	count = 0;

	servers = &cls.globalServers[0];
	count = MAX_GLOBAL_SERVERS;

	if (servers) {
		for (i = 0; i < count; i++) {
			servers[i].ping = -1;
		}
	}
}

static void UI_UpdatePendingPings() { 
	LAN_ResetPings(0);
	uiInfo.serverStatus.refreshActive = true;
	uiInfo.serverStatus.refreshtime = GetTickCount() + 1000;

}


static void UI_StartServerRefresh(qboolean full)
{
	int		i;
	char	*ptr;

	if (!full) {
		UI_UpdatePendingPings();
		return;
	}

	uiInfo.serverStatus.refreshActive = true;
	uiInfo.serverStatus.nextDisplayRefresh = GetTickCount() + 1000;
	// clear number of displayed servers
	uiInfo.serverStatus.numDisplayServers = 0;
	uiInfo.serverStatus.numPlayersOnServers = 0;
	// mark all servers as visible so we store ping updates for them
//	Com_DPrintf(0, "Marking servers as visible.\n");
	LAN_MarkServerVisible(0, -1, true);
	// reset all the pings
	LAN_ResetPings(0);

	uiInfo.serverStatus.refreshtime = GetTickCount() + 5000;

	Cmd_ExecuteString( 0, 0, va( "globalservers %d %d full empty\n", 0, 142 ) );
}

char* UIFeederItemTextHookFunc()
{
	static char info[8192];
	static char hostname[1024];
	static char clientBuff[32];
	static int lastColumn = -1;
	static int lastTime = 0;

	if (currentFeeder == 2.0f)
	{
		//sprintf(sting, "%d-%d", index, column);
		//return sting;
		if (index >= 0 && index < uiInfo.serverStatus.numDisplayServers)
		{
			int ping, game, punkbuster, hardcore;
			if (lastColumn != column || lastTime > GetTickCount() + 5000) {
				LAN_GetServerInfo(0, uiInfo.serverStatus.displayServers[index], info, 8192);
				lastColumn = column;
				lastTime = GetTickCount();
			}
			ping = atoi(Info_ValueForKey(info, "ping"));
			hardcore = atoi(Info_ValueForKey(info, "hc"));
			if (ping == -1) {
				// if we ever see a ping that is out of date, do a server refresh
				// UI_UpdatePendingPings();
			}
			switch (column) {
				case SORT_HOST : 
					if (ping <= 0) {
						return Info_ValueForKey(info, "addr");
					} else {
						/*if ( ui_netSource.integer == AS_LOCAL ) {
							_snprintf( hostname, sizeof(hostname), "%s [%s]",
								Info_ValueForKey(info, "hostname"),
								netnames[atoi(Info_ValueForKey(info, "nettype"))] );
							return hostname;
						}
						else {*/
							_snprintf( hostname, sizeof(hostname), "%s", Info_ValueForKey(info, "hostname"));
							return hostname;
						//}
					}
				case SORT_MAP : return UI_LocalizeMapName(Info_ValueForKey(info, "mapname"));
				case SORT_CLIENTS : 
					_snprintf( clientBuff, sizeof(clientBuff), "%s (%s)", Info_ValueForKey(info, "clients"), Info_ValueForKey(info, "sv_maxclients"));
					return clientBuff;
				case SORT_GAME : 
					return UI_LocalizeGameType(Info_ValueForKey(info, "gametype"));
				case SORT_HARDCORE :
					if (hardcore & 1) {
						return "X";
					} else {
						return "";
					}
				case SORT_MOD :
					if (strlen(Info_ValueForKey(info, "fs_game")) == 0) {
						return "X";
					} else {
						return "";
					}
				case SORT_UPD :
					if (strlen(Info_ValueForKey(info, "shortversion")) != 0) {
						return "X";
					} else {
						return "";
					}
				case SORT_PING : 
					if (ping <= 0) {
						return "...";
					} else {
						return Info_ValueForKey(info, "ping");
					}
			}
		}

		return "";
	}

	return NULL;
}

/*
=================
UI_ServersQsortCompare
=================
*/
static int UI_ServersQsortCompare( const void *arg1, const void *arg2 ) {
	return LAN_CompareServers( 0, uiInfo.serverStatus.sortKey, uiInfo.serverStatus.sortDir, *(int*)arg1, *(int*)arg2);
}


/*
=================
UI_ServersSort
=================
*/
void UI_ServersSort(int column, qboolean force) {

	if ( !force ) {
		if ( uiInfo.serverStatus.sortKey == column ) {
			return;
		}
	}
	Logger(lINFO,"CIserverlist", "Server sort");
	uiInfo.serverStatus.sortKey = column;
	qsort( &uiInfo.serverStatus.displayServers[0], uiInfo.serverStatus.numDisplayServers, sizeof(int), UI_ServersQsortCompare);
}