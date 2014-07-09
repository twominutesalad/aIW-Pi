/*
================================
alterIWnet Client
Version 2.0
================================
*/

/*
Based on Melodia's alterOps Steam_API project and NTAuthority's Steam_API
Please check TODO comments
- No
*/

/*
Working:
- Stats
- Matchmaking
- Mods
- Custom IWD files
- Connect command
- CIClient
- Server list
- Custom stat encoding/decoding
- Matchmaking menu

Not Working:
- ?

Not Implemented:
- Flux
- more?
*/

#include "steam_api.h"
#include "StdAfx.h"
#include <string.h>
#include <sys/stat.h>
#include "IniReader.h"

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
using namespace System::Collections::Generic;
using namespace System::IO;
using namespace System::Xml::Linq;
using namespace System::Net;
using namespace System::Net::NetworkInformation;
using namespace System::Reflection;
using namespace System::Threading;

void UpdateValidation(BOOL isInitial);
void Logger(unsigned int lvl, char* caller, char* logline, ...);

Microsoft::Win32::RegistryKey^ GetNetworkRegistryKey(String^ id) {
	Microsoft::Win32::RegistryKey^ networkInterfaceKey = Microsoft::Win32::Registry::LocalMachine->OpenSubKey("SYSTEM\\CurrentControlSet\\Control\\Class\\{4D36E972-E325-11CE-BFC1-08002BE10318}");
	cli::array<String^>^ keyNames = networkInterfaceKey->GetSubKeyNames();

	for each (String^ keyName in keyNames) {
		Microsoft::Win32::RegistryKey^ key = networkInterfaceKey->OpenSubKey(keyName);

		String^ value = (String^)key->GetValue("NetCfgInstanceId", "");
		if (value == id) {
			return key;
		}
	}

	return nullptr;
}

bool IsValidInterface(String^ id) {
	if (Environment::OSVersion->Platform != PlatformID::Win32Windows && Environment::OSVersion->Platform != PlatformID::Win32NT) {
		return true;
	}

	Microsoft::Win32::RegistryKey^ key = GetNetworkRegistryKey(id);

	if (key == nullptr) {
		return false;
	}

	//String^ deviceID = (String^)key->GetValue("DeviceInstanceId", "");
	String^ deviceID = (String^)key->GetValue("MatchingDeviceId", "");

	return (deviceID->ToLower()->StartsWith("pci"));
}

bool IsConnectedInterface(String^ id) {
	if (Environment::OSVersion->Platform != PlatformID::Win32Windows && Environment::OSVersion->Platform != PlatformID::Win32NT) {
		return true;
	}

	Microsoft::Win32::RegistryKey^ key = GetNetworkRegistryKey(id);

	if (key == nullptr) {
		return false;
	}

	cli::array<String^>^ values = key->GetValueNames();
	String^ valueName = "";

	for each (String^ value in values) {
		if (value->ToLower()->StartsWith("ne") && value->ToLower()->Contains("re")) {
			valueName = value;
		}
	}

	if (valueName == "") {
		return true;
	}

	String^ valueData = (String^)key->GetValue(valueName, "");
	return (valueData == String::Empty);
}

unsigned int steamID = 0;
bool gotFakeSteamID = true;
bool useNewAuthFunctions = true;
bool connectedInterface = true;

unsigned int GetPlayerSteamID() {
	//return 51393034;
	//StreamReader^ reader = File::OpenText("steamID.txt");
	//int id = int::Parse(reader->ReadToEnd()->Trim());
	//reader->Close();
	/*if (useNewAuthFunctions && Custom::Hook != nullptr) {
		int id = Custom::Hook->GetSteamID();

		if (id != 0) {
			return id;
		}
	}*/

	if (steamID == 0) {
		//steamID = Random::Next();
		gotFakeSteamID = true;
		Random^ random = gcnew Random();
		steamID = random->Next();

		#if !DEDICATED
		try {
			cli::array<NetworkInformation::NetworkInterface^>^ ifaces = NetworkInformation::NetworkInterface::GetAllNetworkInterfaces();
			for each (NetworkInterface^ iface in ifaces) {
				if (iface->NetworkInterfaceType != NetworkInterfaceType::Tunnel && iface->NetworkInterfaceType != NetworkInterfaceType::Loopback) {
					if (!IsValidInterface(iface->Id)) {
						continue;
					}

					if (!IsConnectedInterface(iface->Id)) {
						connectedInterface = false;
						continue;
					}
						cli::array<unsigned char>^ address = iface->GetPhysicalAddress()->GetAddressBytes();
						try {
							pin_ptr<unsigned char> addressPtr = &address[0];

							unsigned int value = 0,temp = 0;
							for(size_t i=0;i<address->Length;i++)
							{
								temp = addressPtr[i];
								temp += value;
								value = temp << 10;
								temp += value;
								value = temp >> 6;
								value ^= temp;
							}
							temp = value << 3;
							temp += value;
							unsigned int temp2 = temp >> 11;
							temp = temp2 ^ temp;
							temp2 = temp << 15;
							value = temp2 + temp;
							if(value < 2) value += 2;
							steamID = value;

							// check steamID for being '2', which happens if GetAddressBytes is a list of zero
							if (steamID == 2) {
								continue;
							}

							gotFakeSteamID = false;
							break;
						} catch (IndexOutOfRangeException^) {

						}
				}
			}
		} catch (Exception^) {}
		#endif

	#if !DEDICATED
		String^ filename = Environment::ExpandEnvironmentVariables("%appdata%\\steam_md5.dat");
	#else
		String^ filename = "dedi_xuid.dat";
	#endif
		if (!File::Exists(filename)) {
			FileStream^ stream = File::OpenWrite(filename);
			stream->Write(BitConverter::GetBytes(steamID), 0, 4);
			stream->Close();
		} else {
			if (gotFakeSteamID) {
				/*FileStream^ stream = File::OpenRead(filename);
				array<Byte>^ buffer = gcnew array<Byte>(5);
				stream->Read(buffer, 0, 4);
				steamID = BitConverter::ToUInt32(buffer, 0);
				stream->Close();*/

				steamID = 0x3BADBAD;

				//Windows::Forms::MessageBox::Show("WARNING #5C-DEV-IDGEN: please report on http://alteriw.net/ forums.", "alterCI");
			}
		}
	}

	return steamID;
}

unsigned int GetPlayersID()
{
	//TODO
	//Flux integration
	//int id = 0x13C48F29;
//	int id = 0x23C48F29;
	return GetPlayerSteamID();
}

class CSteamUser012 : public ISteamUser012
{
public:
	HSteamUser GetHSteamUser()
	{
		//Logger(lSAPI, "CSteamUser", "GetHSteamUser");
		return NULL;
	}

	bool LoggedOn()
	{
//		Logger(lSAPI, "CSteamUser", "LoggedOn");
		return true;
	}

	CSteamID GetSteamID()
	{
//		Logger(lSAPI, "CSteamUser", "GetSteamID");
		return CSteamID( GetPlayersID(), 1, k_EUniversePublic, k_EAccountTypeIndividual );
	}

	int InitiateGameConnection( void *pAuthBlob, int cbMaxAuthBlob, CSteamID steamIDGameServer, uint32 unIPServer, uint16 usPortServer, bool bSecure )
	{
		Logger(lSAPI, "CSteamUser", "InitiateGameConnection");
		Logger(lSAPI, "CsteamUser", "IP: %d, Port: %d, SteamID: %s, blob size: %d", unIPServer, usPortServer, steamIDGameServer.Render(), cbMaxAuthBlob);
		unsigned int steamID = GetPlayerSteamID();
		memcpy(pAuthBlob, &steamID, 4);

		return 4;
	}

	void TerminateGameConnection( uint32 unIPServer, uint16 usPortServer )
	{
		Logger(lSAPI, "CSteamUser", "TerminateGameConnection");
	}

	void TrackAppUsageEvent( CGameID gameID, EAppUsageEvent eAppUsageEvent, const char *pchExtraInfo )
	{
		Logger(lSAPI, "CSteamUser", "TrackAppUsageEvent");
	}

	bool GetUserDataFolder( char *pchBuffer, int cubBuffer )
	{
		Logger(lSAPI, "CSteamUser", "GetUserDataFolder");
		strcpy( pchBuffer, g_Directory.GetDirectoryFileA( "steam_data" ) );

		return true;
	}

	void StartVoiceRecording( )
	{
		Logger(lSAPI, "CSteamUser", "StartVoiceRecording");
	}

	void StopVoiceRecording( )
	{
		Logger(lSAPI, "CSteamUser", "StopVoiceRecording");
	}

	EVoiceResult GetCompressedVoice( void *pDestBuffer, uint32 cbDestBufferSize, uint32 *nBytesWritten )
	{
		Logger(lSAPI, "CSteamUser", "GetCompressedVoice");
		return k_EVoiceResultOK;
	}

	EVoiceResult DecompressVoice( void *pCompressed, uint32 cbCompressed, void *pDestBuffer, uint32 cbDestBufferSize, uint32 *nBytesWritten )
	{
		Logger(lSAPI, "CSteamUser", "DecompressVoice");
		return k_EVoiceResultOK;
	}

	HAuthTicket GetAuthSessionTicket( void *pTicket, int cbMaxTicket, uint32 *pcbTicket )
	{
		Logger(lSAPI, "CSteamUser", "GetAuthSessionTicket");
		return 0;
	}

	EBeginAuthSessionResult BeginAuthSession( const void *pAuthTicket, int cbAuthTicket, CSteamID steamID )
	{
		Logger(lSAPI, "CSteamUser", "BeginAuthSession");
		return k_EBeginAuthSessionResultOK;
	}

	void EndAuthSession( CSteamID steamID )
	{
		Logger(lSAPI, "CSteamUser", "EndAuthSession");
	}

	void CancelAuthTicket( HAuthTicket hAuthTicket )
	{
		Logger(lSAPI, "CSteamUser", "CancelAuthTicket");
	}

	uint32 UserHasLicenseForApp( CSteamID steamID, AppId_t appID )
	{
		Logger(lSAPI, "CSteamUser", "UserHasLicenceForApp");
		return 1;
	}
};

/*bool StatExists(const char* strFilename) { 
  struct stat stFileInfo; 
  bool blnReturn; 
  int intStat; 

  char c[255];
  sprintf(c, "players\\%s_aIW.stat", strFilename);

  intStat = stat(c, &stFileInfo); 
  if(intStat == 0) { 
    blnReturn = true; 
  } else { 
    blnReturn = false; 
  } 
   
  return(blnReturn); 
}

int32 StatSize(const char* strFilename) {
	struct stat filestatus;

	char c[255];
	sprintf(c, "players\\%s_aIW.stat", strFilename);
	stat( c, &filestatus );

	return filestatus.st_size;
}*/

class CSteamRemoteStorage002 : public ISteamRemoteStorage002
{
	public:
		bool FileWrite( const char *pchFile, const void *pvData, int32 cubData )
		{
			Logger(lSAPI, "CSteamRemoteStorage", "FileWrite");
			return true;
		}

		int32 GetFileSize( const char *pchFile )
		{
			Logger(lSAPI, "CSteamRemoteStorage", "GetFileSize (%s)", pchFile);
			//int32 size = (int32)StatSize(pchFile);
			return 0;
		}

		int32 FileRead( const char *pchFile, void *pvData, int32 cubDataToRead )
		{
			Logger(lSAPI, "CSteamRemoteStorage", "FileRead");
			return 0;
		}

		bool FileExists( const char *pchFile )
		{
			//Logger(lSAPI, "CSteamRemoteStorage", "FileExists (%s)", pchFile);
			if ( !stricmp(pchFile, "md202ef8d.bak") )
			{
				// Get player data and save it in a txt file (stats.txt)
				// For use with flux
				// -No
				//CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)getPlayerStats, NULL, NULL, NULL);
			}
			//return StatExists(pchFile);
			return false;
		}

		int32 GetFileCount()
		{
			Logger(lSAPI, "CSteamRemoteStorage", "GetFileCount");
			return 0;
		}

		const char *GetFileNameAndSize( int iFile, int32 *pnFileSizeInBytes )
		{
			Logger(lSAPI, "CSteamRemoteStorage", "GetFileNameAndSize");			
			*pnFileSizeInBytes = 0;

			return "";
		}

		bool GetQuota( int32 *pnTotalBytes, int32 *puAvailableBytes )
		{
			Logger(lSAPI, "CSteamRemoteStorage", "GetQuota");

			*pnTotalBytes		= 0x10000000;
			*puAvailableBytes	= 0x10000000;

			return true;
		}
};


char* persName = NULL;
void OpenMenu(char* name);

class CSteamFriends005 : public ISteamFriends005
{
public:
   const char *GetPersonaName()
    {
        //Logger( "ISteamFriends005.log", "GetPersonaName" );
		if (!persName) {
			int id = GetPlayerSteamID();

			CSteamID steamID( /*33068178*/id, 1, k_EUniversePublic, k_EAccountTypeIndividual );

			CIniReader reader(".\\alterIWnet.ini");
			persName = reader.ReadString("Configuration", "Nickname", steamID.Render());

			if (!persName) {
				return "UnknownPlayer";
			}
		}

		return persName;
	}

	void SetPersonaName( const char *pchPersonaName )
	{
		Logger(lSAPI, "CSteamFriends", "SetPersonaName");
	}

	EPersonaState GetPersonaState()
	{
		Logger(lSAPI, "CSteamFriends", "GetPersonaState");
		return k_EPersonaStateOnline;
	}

	int GetFriendCount( EFriendFlags iFriendFlags )
	{
		Logger(lSAPI, "CSteamFriends", "GetFriendCount");
		return 0;
	}

	CSteamID GetFriendByIndex( int iFriend, int iFriendFlags )
	{
		Logger(lSAPI, "CSteamFriends", "GetFriendByIndex");
		return CSteamID();
	}

	EFriendRelationship GetFriendRelationship( CSteamID steamIDFriend )
	{
		Logger(lSAPI, "CSteamFriends", "GetFriendRelationship");
		return k_EFriendRelationshipNone;
	}

	EPersonaState GetFriendPersonaState( CSteamID steamIDFriend )
	{
		Logger(lSAPI, "CSteamFriends", "GetFriendPersonaState");
		return k_EPersonaStateOffline;
	}

	const char *GetFriendPersonaName( CSteamID steamIDFriend )
	{
		Logger(lSAPI, "CSteamFriends", "GetFriendPersonaName");
		return "aIW Friend";
	}

	int GetFriendAvatar( CSteamID steamIDFriend, int eAvatarSize )
	{
		Logger(lSAPI, "CSteamFriends", "GetFriendAvatar");
		return 0;
	}

	bool GetFriendGamePlayed( CSteamID steamIDFriend, FriendGameInfo_t *pFriendGameInfo )
	{
		Logger(lSAPI, "CSteamFriends", "GetFriendGamePlayed");
		return false;
	}

	const char *GetFriendPersonaNameHistory( CSteamID steamIDFriend, int iPersonaName )
	{
		Logger(lSAPI, "CSteamFriends", "GetFriendPersonaNameHistory");
		return "";
	}

	bool HasFriend( CSteamID steamIDFriend, EFriendFlags iFriendFlags )
	{
		Logger(lSAPI, "CSteamFriends", "HasFriend");
		return false;
	}

	int GetClanCount()
	{
		Logger(lSAPI, "CSteamFriends", "GetClanCount");
		return 0;
	}

	CSteamID GetClanByIndex( int iClan )
	{
		Logger(lSAPI, "CSteamFriends", "GetClanByIndex");
		return CSteamID();
	}

	const char *GetClanName( CSteamID steamIDClan )
	{
		Logger(lSAPI, "CSteamFriends", "GetClanName");
		return "aIWDev";
	}

	int GetFriendCountFromSource( CSteamID steamIDSource )
	{
		Logger(lSAPI, "CSteamFriends", "GetFriendCountFromSource");
		return 0;
	}

	CSteamID GetFriendFromSourceByIndex( CSteamID steamIDSource, int iFriend )
	{
		Logger(lSAPI, "CSteamFriends", "GetFriendFromSourceByIndex");
		return CSteamID();
	}

	bool IsUserInSource( CSteamID steamIDUser, CSteamID steamIDSource )
	{
		Logger(lSAPI, "CSteamFriends", "IsUserInSource");
		return false;
	}

	void SetInGameVoiceSpeaking( CSteamID steamIDUser, bool bSpeaking )
	{
		Logger(lSAPI, "CSteamFriends", "SetInGameVoiceSpeaking");
		
	}

	void ActivateGameOverlay( const char *pchDialog )
	{
		Logger(lSAPI, "CSteamFriends", "ActivateGameOverlay (%s)", pchDialog);
		if ( !stricmp(pchDialog, "LobbyInvite") )
		{
			Logger(lDEBUG, "alterIWnet", "Opening server browser menu");
			// Open the server browser menu -No
			OpenMenu("pc_join_unranked");
		}
		
	}

	void ActivateGameOverlayToUser( const char *pchDialog, CSteamID steamID )
	{
		Logger(lSAPI, "CSteamFriends", "ActivateGameOverlayToUser");
		
	}

	void ActivateGameOverlayToWebPage( const char *pchURL )
	{
		Logger(lSAPI, "CSteamFriends", "ActivateGameOverlayToWebPage");
		
	}

	void ActivateGameOverlayToStore( AppId_t nAppID )
	{
		Logger(lSAPI, "CSteamFriends", "ActivateGameOverlayToStore");
		
	}

	void SetPlayedWith( CSteamID steamID )
	{
		Logger(lSAPI, "CSteamFriends", "SetPlayedWith");
		
	}
};

class CSteamNetworking003 : public ISteamNetworking003
{
public:

	bool SendP2PPacket( CSteamID, const void*, uint32, EP2PSend ) 
	{
		Logger(lSAPI, "CSteamNetworking", "SendP2PPacket");
		return false;
	}

	bool IsP2PPacketAvailable( uint32 *a1 ) 
	{
		//Logger(lSAPI, "CSteamNetworking", "IsP2PPacketAvailable");
		return false;
	}

	bool ReadP2PPacket( void *a1, uint32 a2, uint32 *a3, CSteamID *a4 ) 
	{
		Logger(lSAPI, "CSteamNetworking", "ReadP2PPacket");
		return false;
	}

	bool AcceptP2PSessionWithUser( CSteamID ) 
	{
		Logger(lSAPI, "CSteamNetworking", "AcceptP2PSessionWithUser");
		return false;
	}

	bool CloseP2PSessionWithUser( CSteamID ) 
	{
		Logger(lSAPI, "CSteamNetworking", "CloseP2PSessionWithUser");
		return false;
	}

	bool GetP2PSessionState( CSteamID, P2PSessionState_t * ) 
	{
		Logger(lSAPI, "CSteamNetworking", "GetP2PSessionState");
		return false;
	}

	SNetListenSocket_t CreateListenSocket( int nVirtualP2PPort, uint32 nIP, uint16 nPort, bool bAllowUseOfPacketRelay )
	{
		Logger(lSAPI, "CSteamNetworking", "CreateListenSocket");
		return NULL;
	}

	SNetSocket_t CreateP2PConnectionSocket( CSteamID steamIDTarget, int nVirtualPort, int nTimeoutSec, bool bAllowUseOfPacketRelay )
	{
		Logger(lSAPI, "CSteamNetworking", "CreateP2PConnectionSocket");
		return NULL;
	}

	SNetSocket_t CreateConnectionSocket( uint32 nIP, uint16 nPort, int nTimeoutSec )
	{
		Logger(lSAPI, "CSteamNetworking", "CreateConnectionSocket");
		return NULL;
	}

	bool DestroySocket( SNetSocket_t hSocket, bool bNotifyRemoteEnd )
	{
		Logger(lSAPI, "CSteamNetworking", "DestroySocket");
		return false;
	}

	bool DestroyListenSocket( SNetListenSocket_t hSocket, bool bNotifyRemoteEnd )
	{
		Logger(lSAPI, "CSteamNetworking", "DestroyListenSocket");
		return false;
	}

	bool SendDataOnSocket( SNetSocket_t hSocket, void *pubData, uint32 cubData, bool bReliable )
	{
		Logger(lSAPI, "CSteamNetworking", "SendDataOnSocket");
		return false;
	}

	bool IsDataAvailableOnSocket( SNetSocket_t hSocket, uint32 *pcubMsgSize )
	{
		Logger(lSAPI, "CSteamNetworking", "IsDataAvailableOnSocket");
		return false;
	}

	bool RetrieveDataFromSocket( SNetSocket_t hSocket, void *pubDest, uint32 cubDest, uint32 *pcubMsgSize )
	{
		Logger(lSAPI, "CSteamNetworking", "RetrieveDataFromSocket");
		return false;
	}

	bool IsDataAvailable( SNetListenSocket_t hListenSocket, uint32 *pcubMsgSize, SNetSocket_t *phSocket )
	{
		Logger(lSAPI, "CSteamNetworking", "IsDataAvailable");
		return false;
	}

	bool RetrieveData( SNetListenSocket_t hListenSocket, void *pubDest, uint32 cubDest, uint32 *pcubMsgSize, SNetSocket_t *phSocket )
	{
		Logger(lSAPI, "CSteamNetworking", "RetrieveData");
		return false;
	}

	bool GetSocketInfo( SNetSocket_t hSocket, CSteamID *pSteamIDRemote, int *peSocketStatus, uint32 *punIPRemote, uint16 *punPortRemote )
	{
		Logger(lSAPI, "CSteamNetworking", "GetSocketInfo");
		return false;
	}

	bool GetListenSocketInfo( SNetListenSocket_t hListenSocket, uint32 *pnIP, uint16 *pnPort )
	{
		Logger(lSAPI, "CSteamNetworking", "GetListenSocketInfo");
		return false;
	}

	ESNetSocketConnectionType GetSocketConnectionType( SNetSocket_t hSocket )
	{
		Logger(lSAPI, "CSteamNetworking", "GetSocketConnectionType");
		return k_ESNetSocketConnectionTypeNotConnected;
	}

	int GetMaxPacketSize( SNetSocket_t hSocket )
	{
		Logger(lSAPI, "CSteamNetworking", "GetMaxPacketSize");
		return 0 - 0 + (5 - 5);
	}
};

LONGLONG FileTime_to_POSIX(FILETIME ft)
{
	// takes the last modified date
	LARGE_INTEGER date, adjust;
	date.HighPart = ft.dwHighDateTime;
	date.LowPart = ft.dwLowDateTime;

	// 100-nanoseconds = milliseconds * 10000
	adjust.QuadPart = 11644473600000 * 10000;

	// removes the diff between 1970 and 1601
	date.QuadPart -= adjust.QuadPart;

	// converts back from 100-nanoseconds to seconds
	return date.QuadPart / 10000000;
}

class CSteamUtils005 : public ISteamUtils005
{
public:
	uint32 GetSecondsSinceAppActive()
	{
		Logger(lSAPI, "CSteamUtils", "GetSecondsSinceAppActive");
		return 0;
	}

	uint32 GetSecondsSinceComputerActive()
	{
		Logger(lSAPI, "CSteamUtils", "GetSecondsSinceComputerActive");
		return 0;
	}

	EUniverse GetConnectedUniverse()
	{
		Logger(lSAPI, "CSteamUtils", "GetConnectedUniverse");
		return k_EUniversePublic;
	}

	uint32 GetServerRealTime()
	{
		Logger(lSAPI, "CSteamUtils", "GetServerRealTime");
		FILETIME ft;
		SYSTEMTIME st;

		GetSystemTime(&st);
		SystemTimeToFileTime(&st, &ft);

		return (uint32)FileTime_to_POSIX(ft);
	}

	const char *GetIPCountry()
	{
		Logger(lSAPI, "CSteamUtils", "GetIPCountry");
		return "US";
	}

	bool GetImageSize( int iImage, uint32 *pnWidth, uint32 *pnHeight )
	{
		Logger(lSAPI, "CSteamUtils", "GetImageSize");
		return false;
	}

	bool GetImageRGBA( int iImage, uint8 *pubDest, int nDestBufferSize )
	{
		Logger(lSAPI, "CSteamUtils", "GetImageRGBA");
		return false;
	}

	bool GetCSERIPPort( uint32 *unIP, uint16 *usPort )
	{
		Logger(lSAPI, "CSteamUtils", "GetCSERIPPort");
		return false;
	}

	uint8 GetCurrentBatteryPower()
	{
		Logger(lSAPI, "CSteamUtils", "GetCurrentBatteryPower");
		return 255;
	}

	uint32 GetAppID()
	{
		Logger(lSAPI, "CSteamUtils", "GetAppID");
		return 440;
	}

	void SetOverlayNotificationPosition( ENotificationPosition eNotificationPosition )
	{
		Logger(lSAPI, "CSteamUtils", "SetOverlayNotificationPosition (%d)", eNotificationPosition);
	}

	bool IsAPICallCompleted( SteamAPICall_t hSteamAPICall, bool *pbFailed )
	{
		Logger(lSAPI, "CSteamUtils", "IsAPICallCompleted");
		return (Callbacks::_calls->ContainsKey(hSteamAPICall)) ? Callbacks::_calls[hSteamAPICall] : false;
	}

	ESteamAPICallFailure GetAPICallFailureReason( SteamAPICall_t hSteamAPICall )
	{
		Logger(lSAPI, "CSteamUtils", "GetAPICallFailureReason");
		return k_ESteamAPICallFailureNone;
	}

	bool GetAPICallResult( SteamAPICall_t hSteamAPICall, void *pCallback, int cubCallback, int iCallbackExpected, bool *pbFailed )
	{
		Logger(lSAPI, "CSteamUtils", "GetAPICallResult");
		return false;
	}

	void RunFrame()
	{
		Logger(lSAPI, "CSteamUtils", "RunFrame");
	}

	uint32 GetIPCCallCount()
	{
		Logger(lSAPI, "CSteamUtils", "GetIPCCallCount");
		return 0;
	}

	void SetWarningMessageHook( SteamAPIWarningMessageHook_t pFunction )
	{
		Logger(lSAPI, "CSteamUtils", "SetWarningMessageHook");
	}

	bool IsOverlayEnabled()
	{
		Logger(lSAPI, "CSteamUtils", "IsOverlayEnabled");
		return false;
	}

	bool BOverlayNeedsPresent()
	{
		Logger(lSAPI, "CSteamUtils", "BOverlayNeedsPresent");
		return false;
	}

	SteamAPICall_t CheckFileSignature( const char *szFileName )
	{
		Logger(lSAPI, "CSteamUtils", "CheckFileSignature");
		return k_ECheckFileSignatureValidSignature;
	}
};

class CSteamMasterServerUpdater001 : public ISteamMasterServerUpdater001
{
	void SetActive( bool bActive )
	{
		Logger(lSAPI, "CSteamMasterServerUpdater", "SetActive");
	}

	void SetHeartbeatInterval( int iHeartbeatInterval )
	{
		Logger(lSAPI, "CSteamMasterServerUpdater", "SetHeartbeatInterval");
	}

	bool HandleIncomingPacket( const void *pData, int cbData, uint32 srcIP, uint16 srcPort )
	{
		Logger(lSAPI, "CSteamMasterServerUpdater", "HandleIncomingPacket");
		return true;
	}

	int GetNextOutgoingPacket( void *pOut, int cbMaxOut, uint32 *pNetAdr, uint16 *pPort )
	{
	Logger(lSAPI, "CSteamMasterServerUpdater", "GetNextOutgoingPacket");
		return 0;
	}

	void SetBasicServerData(
		unsigned short nProtocolVersion,
		bool bDedicatedServer,
		const char *pRegionName,
		const char *pProductName,
		unsigned short nMaxReportedClients,
		bool bPasswordProtected,
		const char *pGameDescription )
	{
		Logger(lSAPI, "CSteamMasterServerUpdater", "SetBasicServerData");
	}

	void ClearAllKeyValues()
	{
		Logger(lSAPI, "CSteamMasterServerUpdater", "ClearAllKeyValues");
	}

	void SetKeyValue( const char *pKey, const char *pValue )
	{
		Logger(lSAPI, "CSteamMasterServerUpdater", "SetKeyValue");
	}

	void NotifyShutdown()
	{
		Logger(lSAPI, "CSteamMasterServerUpdater", "NotifyShutdown");
	}

	bool WasRestartRequested()
	{
		Logger(lSAPI, "CSteamMasterServerUpdater", "WasRestartRequested");
		return false;
	}

	void ForceHeartbeat()
	{
		Logger(lSAPI, "CSteamMasterServerUpdater", "ForceHeartbeat");
	}

	bool AddMasterServer( const char *pServerAddress ) 
	{
		Logger(lSAPI, "CSteamMasterServerUpdater", "AddMasterServer");
		return true;
	}

	bool RemoveMasterServer( const char *pServerAddress )
	{
		Logger(lSAPI, "CSteamMasterServerUpdater", "RemoveMasterServer");
		return true;
	}

	int GetNumMasterServers()
	{
		Logger(lSAPI, "CSteamMasterServerUpdater", "GetNumMasterServers");
		return 0;
	}

	int GetMasterServerAddress( int iServer, char *pOut, int outBufferSize )
	{
		Logger(lSAPI, "CSteamMasterServerUpdater", "GetMasterServerAddress");
		return 0;
	}

	unknown_ret SetPort( uint16 ) { return 0; }
	unknown_ret DontUseMe() { return 0; }
	unknown_ret OnUDPFatalError( uint32, uint32 ) { return 0; }
};

bool isInConnectCMD = false;
int newLobbyFakeID = 9999999;
int lobbyIP = 0;
int lobbyPort = 0;
int lobbyIPLoc = 0;
int lobbyPortLoc = 0;

char lobbyIPBuf[1024];
char lobbyPortBuf[1024];
char lobbyIPLocBuf[1024];
char lobbyPortLocBuf[1024];



class CSteamMatchmaking007 : public ISteamMatchmaking007
{
public:
	int GetFavoriteGameCount()
	{
		Logger(lSAPI, "CSteamMatchmaking", "GetFavoriteGameCount");
		return 0;
	}

	bool GetFavoriteGame( int iGame, AppId_t *pnAppID, uint32 *pnIP, uint16 *pnConnPort, uint16 *pnQueryPort, uint32 *punFlags, RTime32 *pRTime32LastPlayedOnServer )
	{
		Logger(lSAPI, "CSteamMatchmaking", "GetFavoriteGame");
		return 0;
	}

	int AddFavoriteGame( AppId_t nAppID, uint32 nIP, uint16 nConnPort, uint16 nQueryPort, uint32 unFlags, RTime32 rTime32LastPlayedOnServer )
	{
		Logger(lSAPI, "CSteamMatchmaking", "AddFavoriteGame");
		return 0;
	}

    bool RemoveFavoriteGame( AppId_t nAppID, uint32 nIP, uint16 nConnPort, uint16 nQueryPort, uint32 unFlags )
	{
		Logger(lSAPI, "CSteamMatchmaking", "RemoveFavoriteGame");
		return false;
	}

	SteamAPICall_t RequestLobbyList()
	{
		Logger(lSAPI, "CSteamMatchmaking", "RequestLobbyList");
		
		SteamAPICall_t result = Callbacks::RegisterCall();
		LobbyMatchList_t* retvals = (LobbyMatchList_t*)malloc(sizeof(LobbyMatchList_t));
		retvals->m_nLobbiesMatching = 0;

		Callbacks::Return(retvals, LobbyMatchList_t::k_iCallback, result, sizeof(LobbyMatchList_t));

		return result;
	}

	void AddRequestLobbyListFilter( const char *pchKeyToMatch, const char *pchValueToMatch )
	{
		Logger(lSAPI, "CSteamMatchmaking", "AddRequestLobbyListFilter");
	}

	void AddRequestLobbyListNumericalFilter( const char *pchKeyToMatch, int nValueToMatch, ELobbyComparison nComparisonType )
	{
		Logger(lSAPI, "CSteamMatchmaking", "AddRequestLobbyListNumericalFilter");
	}

	void AddRequestLobbyListNearValueFilter( const char *pchKeyToMatch, int nValueToBeCloseTo )
	{
		Logger(lSAPI, "CSteamMatchmaking", "AddRequestLobbyListNearValueFilter");
	}

	void AddRequestLobbyListSlotsAvailableFilter( int )
	{
		Logger(lSAPI, "CSteamMatchmaking", "AddRequestLobbyListSlotsAvailableFilter");
	}

	void AddRequestLobbyListStringFilter( const char *pchKeyToMatch, const char *pchValueToMatch, ELobbyComparison eComparisonType )
	{
		Logger(lSAPI, "CSteamMatchmaking", "AddRequestLobbyListStringFilter");
	}

	virtual void AddRequestLobbyListFilterSlotsAvailable( int nSlotsAvailable )
	{
		Logger(lSAPI, "CSteamMatchmaking", "AddRequestLobbyListFilterSlotsAvailable");
	}

	CSteamID GetLobbyByIndex( int iLobby )
	{
		Logger(lSAPI, "CSteamMatchmaking", "GetLobbyByIndex");
		return CSteamID( 10293223, 0x40000, k_EUniversePublic, k_EAccountTypeChat );
	}

	SteamAPICall_t CreateLobby( ELobbyType eLobbyType, int cMaxMembers ) 
	{
		SteamAPICall_t result = Callbacks::RegisterCall();
		LobbyCreated_t* retvals = (LobbyCreated_t*)malloc(sizeof(LobbyCreated_t));
		CSteamID id = CSteamID( 10293223, 0x40000, k_EUniversePublic, k_EAccountTypeChat );

		retvals->m_eResult = k_EResultOK;
		retvals->m_ulSteamIDLobby = id.ConvertToUint64();

		Logger(lSAPI, "CSteamMatchmaking", "CreateLobby (%llu)", id.ConvertToUint64());

		Callbacks::Return(retvals, LobbyCreated_t::k_iCallback, result, sizeof(LobbyCreated_t));

		JoinLobby(id);

		return result;
	}

	SteamAPICall_t JoinLobby( CSteamID steamIDLobby ) 
	{
		Logger(lSAPI, "CSteamMatchmaking", "JoinLobby (%llu)", steamIDLobby.ConvertToUint64());

		SteamAPICall_t result = Callbacks::RegisterCall();
		LobbyEnter_t* retvals = (LobbyEnter_t*)malloc(sizeof(LobbyEnter_t));
		retvals->m_bLocked = false;
		retvals->m_EChatRoomEnterResponse = k_EChatRoomEnterResponseSuccess;
		retvals->m_rgfChatPermissions = (EChatPermission)0xFFFFFFFF;
		retvals->m_ulSteamIDLobby = steamIDLobby.ConvertToUint64();

		Callbacks::Return(retvals, LobbyEnter_t::k_iCallback, result, sizeof(LobbyEnter_t));

		return result;
	}

	void LeaveLobby( CSteamID steamIDLobby )
	{
		Logger(lSAPI, "CSteamMatchmaking", "LeaveLobby");
	}

	bool InviteUserToLobby( CSteamID steamIDLobby, CSteamID steamIDInvitee )
	{
		Logger(lSAPI, "CSteamMatchmaking", "InviteUserToLobby");
		return false;
	}

	int GetNumLobbyMembers( CSteamID steamIDLobby )
	{
		//Logger(lSAPI, "CSteamMatchmaking", "GetNumLobbyMembers");
		return 1;
	}

	CSteamID GetLobbyMemberByIndex( CSteamID steamIDLobby, int iMember )
	{
		//Logger(lSAPI, "CSteamMatchmaking", "GetLobbyMemberByIndex (%d)", iMember);
		return CSteamID( GetPlayersID(), 1, k_EUniversePublic, k_EAccountTypeIndividual );
	}

	const char *GetLobbyData( CSteamID steamIDLobby, const char *pchKey )
	{
		Logger(lSAPI, "CSteamMatchmaking", "GetLobbyData (%llu)", steamIDLobby.ConvertToUint64());
		if (!strcmp(pchKey, "addr")) {
			sprintf(lobbyIPBuf, "%d", lobbyIP);
			return lobbyIPBuf;
		}

		if (!strcmp(pchKey, "port")) {
			sprintf(lobbyPortBuf, "%d", lobbyPort);
			return lobbyPortBuf;
		}

		if (!strcmp(pchKey, "addrLoc")) {
			sprintf(lobbyIPLocBuf, "%d", lobbyIPLoc);
			return lobbyIPLocBuf;
		}

		if (!strcmp(pchKey, "portLoc")) {
			sprintf(lobbyPortLocBuf, "%d", lobbyPortLoc);
			return lobbyPortLocBuf;
		}

		return "21212";
	}

	bool SetLobbyData( CSteamID steamIDLobby, const char *pchKey, const char *pchValue )
	{
		Logger(lSAPI, "CSteamMatchmaking", "SetLobbyData (%s)", pchValue);
		SteamAPICall_t result = Callbacks::RegisterCall();
		LobbyDataUpdate_t* retvals = (LobbyDataUpdate_t*)malloc(sizeof(LobbyDataUpdate_t));
		retvals->m_ulSteamIDMember = steamIDLobby.ConvertToUint64();
		retvals->m_ulSteamIDLobby = steamIDLobby.ConvertToUint64();

		Callbacks::Return(retvals, LobbyDataUpdate_t::k_iCallback, result, sizeof(LobbyDataUpdate_t));
		
		return (result)?true:false;
	}

	int GetLobbyDataCount( CSteamID steamIDLobby )
	{
		Logger(lSAPI, "CSteamMatchmaking", "GetLobbyDataCount");
		return 0;
	}

	bool GetLobbyDataByIndex( CSteamID steamIDLobby, int iData, char *pchKey, int cubKey, char *pchValue, int cubValue )
	{
		Logger(lSAPI, "CSteamMatchmaking", "GetLobbyDataByIndex");
		return false;
	}

	bool DeleteLobbyData( CSteamID steamIDLobby, const char *pchKey ) 
	{
		Logger(lSAPI, "CSteamMatchmaking", "DeleteLobbyData");
		return 0;
	}

	const char *GetLobbyMemberData( CSteamID steamIDLobby, CSteamID steamIDUser, const char *pchKey )
	{
		Logger(lSAPI, "CSteamMatchmaking", "GetLobbyMemberData");
		return "";
	}

	void SetLobbyMemberData( CSteamID steamIDLobby, const char *pchKey, const char *pchValue )
	{
		Logger(lSAPI, "CSteamMatchmaking", "SetLobbyMemberData");
	}

	bool SendLobbyChatMsg( CSteamID steamIDLobby, const void *pvMsgBody, int cubMsgBody )
	{
		Logger(lSAPI, "CSteamMatchmaking", "SendLobbyChatMsg");
		return true;
	}

	int GetLobbyChatEntry( CSteamID steamIDLobby, int iChatID, CSteamID *pSteamIDUser, void *pvData, int cubData, EChatEntryType *peChatEntryType )
	{
		Logger(lSAPI, "CSteamMatchmaking", "GetLobbyChatEntry");
		return 0;
	}

	bool RequestLobbyData( CSteamID steamIDLobby )
	{
		Logger(lSAPI, "CSteamMatchmaking", "RequestLobbyData");
		return false;
	}

	void SetLobbyGameServer( CSteamID steamIDLobby, uint32 unGameServerIP, uint16 unGameServerPort, CSteamID steamIDGameServer )
	{
		Logger(lSAPI, "CSteamMatchmaking", "SetLobbyGameServer");
	}

	bool GetLobbyGameServer( CSteamID steamIDLobby, uint32 *punGameServerIP, uint16 *punGameServerPort, CSteamID *psteamIDGameServer )
	{
		Logger(lSAPI, "CSteamMatchmaking", "GetLobbyGameServer");
		return false;
	}

	bool SetLobbyMemberLimit( CSteamID steamIDLobby, int cMaxMembers )
	{
		Logger(lSAPI, "CSteamMatchmaking", "SetLobbyMemberLimit (%d)", cMaxMembers);
		return true;
	}

	int GetLobbyMemberLimit( CSteamID steamIDLobby )
	{
		//Logger(lSAPI, "CSteamMatchmaking", "GetLobbyMemberLimit");
		return 1;
	}

	bool SetLobbyType( CSteamID steamIDLobby, ELobbyType eLobbyType )
	{
		Logger(lSAPI, "CSteamMatchmaking", "SetLobbyType (%d)", steamIDLobby.ConvertToUint64(), eLobbyType);
		return true;
	}

	bool SetLobbyJoinable( CSteamID steamIDLobby, bool bJoinable )
	{
		Logger(lSAPI, "CSteamMatchmaking", "SetLobbyJoinable");
		return true;
	}

	CSteamID GetLobbyOwner( CSteamID steamIDLobby )
	{
		//Logger(lSAPI, "CSteamMatchmaking", "GetLobbyOwner");
		return CSteamID( GetPlayersID(), 1, k_EUniversePublic, k_EAccountTypeIndividual );
		//return CSteamID( 0x100000, k_EUniversePublic, k_EAccountTypeIndividual );
	}

	bool SetLobbyOwner( CSteamID steamIDLobby, CSteamID steamIDOwner ) 
	{
		Logger(lSAPI, "CSteamMatchmaking", "SetLobbyOwner (%llu)", steamIDOwner.ConvertToUint64());
		return true;
	}
};

class CSteamApps003 : public ISteamApps003
{
public:
	bool IsSubscribed()
	{
		Logger(lSAPI, "CSteamApps", "IsSubscribed");
		return true;
	}

	bool IsLowViolence()
	{
		Logger(lSAPI, "CSteamApps", "IsLowViolence");
		return false;
	}

	bool IsCybercafe()
	{
		Logger(lSAPI, "CSteamApps", "IsCybercafe");
		return false;
	}

	bool IsVACBanned()
	{
		Logger(lSAPI, "CSteamApps", "IsVACBanned");
		return false;
	}

	const char *GetCurrentGameLanguage()
	{
		Logger(lSAPI, "CSteamApps", "GetCurrentGameLanguage");
		return "english";
	}

	const char *GetAvailableGameLanguages()
	{
		Logger(lSAPI, "CSteamApps", "GetAvailableGameLanguages");
		return "english";
	}

	bool IsSubscribedApp( AppId_t appID )
	{
		Logger(lSAPI, "CSteamApps", "IsSubscribedApp");
		return true;
	}

	bool IsDlcInstalled( AppId_t appID )
	{
		Logger(lSAPI, "CSteamApps", "IsDlcInstalled");
		return true;
	}
};

int failureCount = 0;

void ValidateConnection(Object^ state)
{
	ServicePointManager::Expect100Continue = false;

	CIniReader reader(".\\alterIWnet.ini");
	const char* serverName = reader.ReadString("Configuration", "Server", "log1.pc.iw4.iwnet.infinityward.com");

	String^ hostName = gcnew String(serverName);

	CSteamID steamID = CSteamID((uint64)state);

	try
	{
		WebClient^ wc = gcnew WebClient();
		String^ url = String::Format("http://{0}:13000/clean/{1}", hostName, steamID.ConvertToUint64().ToString());
		String^ result = wc->DownloadString(url);

		if (result == "invalid")
		{
			GSClientDeny_t* retvals = (GSClientDeny_t*)malloc(sizeof(GSClientDeny_t));

			retvals->m_SteamID = steamID;
			retvals->m_eDenyReason = k_EDenyNoLicense;//k_EDenyIncompatibleSoftware;

			Callbacks::Return(retvals, GSClientDeny_t::k_iCallback, 0, sizeof(GSClientDeny_t));
		}
		else
		{
			GSClientApprove_t* retvals = (GSClientApprove_t*)malloc(sizeof(GSClientApprove_t));

			retvals->m_SteamID = steamID;

			Callbacks::Return(retvals, GSClientApprove_t::k_iCallback, 0, sizeof(GSClientApprove_t));
		}
	}
	catch (Exception^)
	{
		failureCount++;

		if (failureCount > 5)
		{
			// as a last resort, just allow them
			GSClientApprove_t* retvals = (GSClientApprove_t*)malloc(sizeof(GSClientApprove_t));

			retvals->m_SteamID = steamID;

			Callbacks::Return(retvals, GSClientApprove_t::k_iCallback, 0, sizeof(GSClientApprove_t));
		}
	}
}

class CSteamGameServer009 : public ISteamGameServer009
{
public:
	void LogOn()
	{
		Logger(lSAPI, "CSteamGameServer", "LogOn");
	}

	void LogOff()
	{
		Logger(lSAPI, "CSteamGameServer", "LogOff");
	}
	
	bool LoggedOn()
	{
		Logger(lSAPI, "CSteamGameServer", "LoggedOn");
		return true;
	}

	bool Secure()
	{
		Logger(lSAPI, "CSteamGameServer", "Secure");
		return false;
	}

	CSteamID GetSteamID()
	{
		Logger(lSAPI, "CSteamGameServer", "GetSteamID");
		return CSteamID( 1337, k_EUniversePublic, k_EAccountTypeIndividual );
	}

	bool SendUserConnectAndAuthenticate( uint32 unIPClient, const void *pvAuthBlob, uint32 cubAuthBlobSize, CSteamID *pSteamIDUser )
	{
		int steamID = *(int*)pvAuthBlob;
		CSteamID realID = CSteamID( steamID, 1, k_EUniversePublic, k_EAccountTypeIndividual );

		ThreadPool::QueueUserWorkItem(gcnew WaitCallback(ValidateConnection), realID.ConvertToUint64());

		*pSteamIDUser = realID;
		return true;
	}

	CSteamID CreateUnauthenticatedUserConnection()
	{
		Logger(lSAPI, "CSteamGameServer", "CreateUnauthenticatedUserConnection");
		return CSteamID( 1337, k_EUniversePublic, k_EAccountTypeIndividual );
	}

	void SendUserDisconnect( CSteamID steamIDUser )
	{
		Logger(lSAPI, "CSteamGameServer", "SendUserDisconnect");
		return;
	}

	bool UpdateUserData( CSteamID steamIDUser, const char *pchPlayerName, uint32 uScore )
	{
		Logger(lSAPI, "CSteamGameServer", "UpdateUserData");
		return true;
	}

	bool SetServerType( uint32 unServerFlags, uint32 unGameIP, uint16 unGamePort, 
								uint16 unSpectatorPort, uint16 usQueryPort, const char *pchGameDir, const char *pchVersion, bool bLANMode )
	{
		Logger(lSAPI, "CSteamGameServer", "SetServerType");
		return true;
	}

	void UpdateServerStatus( int cPlayers, int cPlayersMax, int cBotPlayers, 
									 const char *pchServerName, const char *pSpectatorServerName, 
									 const char *pchMapName )
	{
		Logger(lSAPI, "CSteamGameServer", "UpdateServerStatus");
		return;
	}

	void UpdateSpectatorPort( uint16 unSpectatorPort )
	{
		Logger(lSAPI, "CSteamGameServer", "UpdateSpectatorPort");
		return;
	}

	void SetGameType( const char *pchGameType )
	{
		Logger(lSAPI, "CSteamGameServer", "SetGameType");
		return;
	}

	bool GetUserAchievementStatus( CSteamID steamID, const char *pchAchievementName )
	{
		Logger(lSAPI, "CSteamGameServer", "GetUserAchievementStatus");
		return true;
	}

	void GetGameplayStats( )
	{
		Logger(lSAPI, "CSteamGameServer", "GetGameplayStats");
		return;
	}

	bool RequestUserGroupStatus( CSteamID steamIDUser, CSteamID steamIDGroup )
	{
		Logger(lSAPI, "CSteamGameServer", "RequestUserGroupStatus");
		return true;
	}

	uint32 GetPublicIP()
	{
		Logger(lSAPI, "CSteamGameServer", "GetPublicIP");
		return NULL;
	}

	void SetGameData( const char *pchGameData )
	{
		Logger(lSAPI, "CSteamGameServer", "SetGameData");
		return;
	}

	EUserHasLicenseForAppResult UserHasLicenseForApp( CSteamID steamID, AppId_t appID )
	{
		Logger(lSAPI, "CSteamGameServer", "UserHasLicenseForApp");
		return k_EUserHasLicenseResultHasLicense;
	}
};

#pragma unmanaged

/*class CSteamMatchmakingServers002 : public ISteamMatchmakingServers002
{
public:
	void *RequestInternetServerList( AppId_t iApp, MatchMakingKeyValuePair_t **ppchFilters, uint32 nFilters, ISteamMatchmakingServerListResponse *pRequestServersResponse )
	{
		Logger(lSAPI, "CSteamMatchmakingServers", "RequestInternetServerList");
		return NULL;
	}

	void *RequestLANServerList( AppId_t iApp, ISteamMatchmakingServerListResponse *pRequestServersResponse )
	{
		Logger(lSAPI, "CSteamMatchmakingServers", "RequestLANServerList");
		return NULL;
	}

	void *RequestFriendsServerList( AppId_t iApp, MatchMakingKeyValuePair_t **ppchFilters, uint32 nFilters, ISteamMatchmakingServerListResponse *pRequestServersResponse )
	{
		Logger(lSAPI, "CSteamMatchmakingServers", "RequestFriendsServerList");
		return NULL;
	}

	void *RequestFavoritesServerList( AppId_t iApp, MatchMakingKeyValuePair_t **ppchFilters, uint32 nFilters, ISteamMatchmakingServerListResponse *pRequestServersResponse )
	{
		Logger(lSAPI, "CSteamMatchmakingServers", "RequestFavoritesServerList");
		return NULL;
	}

	void *RequestHistoryServerList( AppId_t iApp, MatchMakingKeyValuePair_t **ppchFilters, uint32 nFilters, ISteamMatchmakingServerListResponse *pRequestServersResponse )
	{
		Logger(lSAPI, "CSteamMatchmakingServers", "RequestHistoryServerList");
		return NULL;
	}

	void *RequestSpectatorServerList( AppId_t iApp, MatchMakingKeyValuePair_t **ppchFilters, uint32 nFilters, ISteamMatchmakingServerListResponse *pRequestServersResponse )
	{
		Logger(lSAPI, "CSteamMatchmakingServers", "RequestSpectatorServerList");
		return NULL;
	}

	void ReleaseRequest( void *pRequest )
	{
		Logger(lSAPI, "CSteamMatchmakingServers", "ReleaseRequest");
		return;
	}

	gameserveritem_t *GetServerDetails( void *pRequest, int iServer )
	{
		Logger(lSAPI, "CSteamMatchmakingServers", "GetServerDetails");
		return NULL;
	}

	void CancelQuery( void *pRequest )
	{
		Logger(lSAPI, "CSteamMatchmakingServers", "CancelQuery");
		return;
	}

	void RefreshQuery( void *pRequest )
	{
		Logger(lSAPI, "CSteamMatchmakingServers", "RefreshQuery");
		return;
	}

	bool IsRefreshing( void *pRequest )
	{
		Logger(lSAPI, "CSteamMatchmakingServers", "IsRefreshing");
		return false;
	}

	int GetServerCount( void *pRequest )
	{
		Logger(lSAPI, "CSteamMatchmakingServers", "GetServerCount");
		return 0;
	}

	void RefreshServer( void *pRequest, int iServer )
	{
		Logger(lSAPI, "CSteamMatchmakingServers", "RefreshServer");
		return;
	}

	HServerQuery PingServer( uint32 unIP, uint16 usPort, ISteamMatchmakingPingResponse *pRequestServersResponse )
	{
		Logger(lSAPI, "CSteamMatchmakingServers", "PingServer");
		return 0;
	}

	HServerQuery PlayerDetails( uint32 unIP, uint16 usPort, ISteamMatchmakingPlayersResponse *pRequestServersResponse )
	{
		Logger(lSAPI, "CSteamMatchmakingServers", "PlayerDetails");
		return 0;
	}

	HServerQuery ServerRules( uint32 unIP, uint16 usPort, ISteamMatchmakingRulesResponse *pRequestServersResponse )
	{
		Logger(lSAPI, "CSteamMatchmakingServers", "ServerRules");
		return 0;
	}

	void CancelServerQuery( HServerQuery hServerQuery )
	{
		Logger(lSAPI, "CSteamMatchmakingServers", "CancelServerQuery");
		return;
	}
};*/

/*
ref class StatBackend {
private:
	static XDocument^ statDocument;
	static XElement^ rootElement;
public:
	static StatBackend() {
		statDocument = gcnew XDocument();
		Load();
	}

	static void Load() {
		if (!File::Exists("emu_stats.xml")) {
			statDocument = gcnew XDocument();
			rootElement = gcnew XElement("Stats");
			statDocument->Add(rootElement);

			statDocument->Save("emu_stats.xml");
		}

		g_Logging.AddToLogFileA("steam_emu.log", "-> created stats file");
		statDocument = XDocument::Load("emu_stats.xml");
		rootElement = statDocument->Root;
	}

	static int32 GetInt(const char* name) {
		String^ nameb = gcnew String(name);

		for each (XElement^ element in rootElement->Descendants("Stat")) {
			if (element->Attribute("Name")->Value == nameb) {
				return int::Parse(element->Value);
			}
		}

		return 0;
	}

	static float GetFloat(const char* name) {
		String^ nameb = gcnew String(name);

		for each (XElement^ element in rootElement->Descendants("Stat")) {
			if (element->Attribute("Name")->Value == nameb) {
				return float::Parse(element->Value);
			}
		}

		return 0.0f;
	}

	static void Set(const char* name, int32 value) {
		String^ nameb = gcnew String(name);

		for each (XElement^ element in rootElement->Descendants("Stat")) {
			if (element->Attribute("Name")->Value == nameb) {
				element->Value = value.ToString();
				return;
			}
		}

		XElement^ stat = gcnew XElement("Stat",
			gcnew XAttribute("Name", nameb)
		);

		stat->Value = value.ToString();
		rootElement->Add(stat);
	}

	static void Set(const char* name, float value) {
		String^ nameb = gcnew String(name);

		for each (XElement^ element in rootElement->Descendants("Stat")) {
			if (element->Attribute("Name")->Value == nameb) {
				element->Value = value.ToString();
				return;
			}
		}

		XElement^ stat = gcnew XElement("Stat",
			gcnew XAttribute("Name", nameb)
		);

		stat->Value = value.ToString();
		rootElement->Add(stat);
	}

	static void Save() {
		statDocument->Save("emu_stats.xml");
	}
};*/


class CSteamUserStats006 : public ISteamUserStats006
{
public:
	bool RequestCurrentStats()
	{
		Logger(lSAPI, "CSteamUserStats", "RequestCurrentStats");
		return true;
	}

	bool GetStat( const char *pchName, int32 *pData )
	{
		Logger(lSAPI, "CSteamUserStats", "GetStat");
		Logger(lSAPI, "CSteamUserStats", (char*)pchName);
		//*pData = StatBackend::GetInt(pchName);

		return true;
	}

	bool GetStat( const char *pchName, float *pData )
	{
		Logger(lSAPI, "CSteamUserStats", "GetStat");
		Logger(lSAPI, "CSteamUserStats", (char*)pchName);
		//*pData = StatBackend::GetFloat(pchName);
	
		return true;
	}

	bool SetStat( const char *pchName, int32 nData )
	{
		Logger(lSAPI, "CSteamUserStats", "SetStat");
		Logger(lSAPI, "CSteamUserStats", (char*)pchName);

		//StatBackend::Set(pchName, nData);
		return true;
	}

	bool SetStat( const char *pchName, float fData )
	{
		Logger(lSAPI, "CSteamUserStats", "SetStat");
		Logger(lSAPI, "CSteamUserStats", (char*)pchName);

		//StatBackend::Set(pchName, fData);
		return true;
	}

	bool UpdateAvgRateStat( const char *pchName, float flCountThisSession, double dSessionLength )
	{
		Logger(lSAPI, "CSteamUserStats", "UpdateAvgRateStat");
		return true;
	}

	bool GetAchievement( const char *pchName, bool *pbAchieved )
	{
		Logger(lSAPI, "CSteamUserStats", "GetAchievement");
		Logger(lSAPI, "CSteamUserStats", (char*)pchName);

		//*pbAchieved = (StatBackend::GetInt(pchName) == 1);

		return true;
	}

	bool SetAchievement( const char *pchName )
	{
		Logger(lSAPI, "CSteamUserStats", "SetAchievement");
		Logger(lSAPI, "CSteamUserStats", (char*)pchName);

		//if (StatBackend::GetInt(pchName) == 0) {
			//PlaySound(L"miscom.wav", NULL, SND_ASYNC | SND_FILENAME);
		//}

		Logger(lSAPI, "CSteamUserStats", "SetAchievement_doing");

		//StatBackend::Set(pchName, 1);
		return true;
	}

	bool ClearAchievement( const char *pchName )
	{
		Logger(lSAPI, "CSteamUserStats", "ClearAchievement");
		Logger(lSAPI, "CSteamUserStats", (char*)pchName);

		//StatBackend::Set(pchName, 0);
		return true;
	}

	bool StoreStats()
	{
		Logger(lSAPI, "CSteamUserStats", "StoreStats");
		//StatBackend::Save();
		return true;
	}

	int GetAchievementIcon( const char *pchName )
	{
		Logger(lSAPI, "CSteamUserStats", "GetAchievementIcon");
		return 0;
	}

	const char *GetAchievementDisplayAttribute( const char *pchName, const char *pchKey )
	{
		Logger(lSAPI, "CSteamUserStats", "GetAchievementDisplayAttribute");
		return "";
	}

	bool IndicateAchievementProgress( const char *pchName, uint32 nCurProgress, uint32 nMaxProgress )
	{
		Logger(lSAPI, "CSteamUserStats", "IndicateAchievementProgress");
		return true;
	}

	SteamAPICall_t RequestUserStats( CSteamID steamIDUser )
	{
		Logger(lSAPI, "CSteamUserStats", "RequestUserStats");
		return NULL;
	}
	
	bool GetUserStat( CSteamID steamIDUser, const char *pchName, int32 *pData )
	{
		Logger(lSAPI, "CSteamUserStats", "GetUserStat");
		return false;
	}

	bool GetUserStat( CSteamID steamIDUser, const char *pchName, float *pData )
	{
		Logger(lSAPI, "CSteamUserStats", "GetUserStat");
		return false;
	}

	bool GetUserAchievement( CSteamID steamIDUser, const char *pchName, bool *pbAchieved )
	{
		Logger(lSAPI, "CSteamUserStats", "GetUserAchievement");
		return false;
	}

	bool ResetAllStats( bool bAchievementsToo )
	{
		Logger(lSAPI, "CSteamUserStats", "ResetAllStats");
		return true;
	}

	SteamAPICall_t FindOrCreateLeaderboard( const char *pchLeaderboardName, ELeaderboardSortMethod eLeaderboardSortMethod, ELeaderboardDisplayType eLeaderboardDisplayType ) 
	{
		Logger(lSAPI, "CSteamUserStats", "FindOrCreateLeaderboard");
		return NULL;
	}
	
	SteamAPICall_t FindLeaderboard( const char *pchLeaderboardName )
	{
		Logger(lSAPI, "CSteamUserStats", "FindLeaderboard");
		return NULL;
	}

	const char *GetLeaderboardName( SteamLeaderboard_t hSteamLeaderboard )
	{
		Logger(lSAPI, "CSteamUserStats", "GetLeaderboardName");
		return "";
	}

	int GetLeaderboardEntryCount( SteamLeaderboard_t hSteamLeaderboard )
	{
		Logger(lSAPI, "CSteamUserStats", "GetLeaderboardEntryCount");
		return 0;
	}

	ELeaderboardSortMethod GetLeaderboardSortMethod( SteamLeaderboard_t hSteamLeaderboard )
	{
		Logger(lSAPI, "CSteamUserStats", "GetLeaderboardSortMethod");
		return k_ELeaderboardSortMethodNone;
	}

	ELeaderboardDisplayType GetLeaderboardDisplayType( SteamLeaderboard_t hSteamLeaderboard )
	{
		Logger(lSAPI, "CSteamUserStats", "GetLeaderboardDisplayType");
		return k_ELeaderboardDisplayTypeNone;
	}

	SteamAPICall_t DownloadLeaderboardEntries( SteamLeaderboard_t hSteamLeaderboard, ELeaderboardDataRequest eLeaderboardDataRequest, int nRangeStart, int nRangeEnd )
	{
		Logger(lSAPI, "CSteamUserStats", "DownloadLeaderboardEntries");
		return NULL;
	}
	
	bool GetDownloadedLeaderboardEntry( SteamLeaderboardEntries_t hSteamLeaderboardEntries, int index, LeaderboardEntry_t *pLeaderboardEntry, int32 *pDetails, int cDetailsMax )
	{
		Logger(lSAPI, "CSteamUserStats", "GetDownloadedLeaderboardEntry");
		return false;
	}

	SteamAPICall_t UploadLeaderboardScore( SteamLeaderboard_t hSteamLeaderboard,ELeaderboardUploadScoreMethod eLeaderboardUploadScoreMethod, int32 nScore, int32 *pScoreDetails, int cScoreDetailsCount )
	{
		Logger(lSAPI, "CSteamUserStats", "UploadLeaderboardScore");
		return NULL;
	}

	SteamAPICall_t GetNumberOfCurrentPlayers()
	{
		Logger(lSAPI, "CSteamUserStats", "GetNumberOfCurrentPlayers");
		return 1;
	}

	bool GetDownloadedLeaderboardEntry( SteamLeaderboardEntries_t hSteamLeaderboardEntries, int index, LeaderboardEntry001_t *pLeaderboardEntry, int32 *pDetails, int cDetailsMax )
	{
		Logger(lSAPI, "CSteamUserStats", "GetDownloadedLeaderboardEntry");
		return false;
	}
};

CSteamUser012*					g_pSteamUserEmu				= new CSteamUser012;
CSteamRemoteStorage002*			g_pSteamRemoteStorageEmu	= new CSteamRemoteStorage002;
CSteamFriends005*				g_pSteamFriendsEmu			= new CSteamFriends005;
CSteamNetworking003*			g_pSteamNetworkingEmu		= new CSteamNetworking003;
CSteamMatchmaking007*			g_pSteamMatchmakingEmu		= new CSteamMatchmaking007;
CSteamMasterServerUpdater001*	g_pSteamMasterServerUpEmu	= new CSteamMasterServerUpdater001;
CSteamUtils005*					g_pSteamUtilsEmu			= new CSteamUtils005;
CSteamGameServer009*			g_pSteamGameServerEmu		= new CSteamGameServer009;
CSteamApps003*					g_pSteamAppsEmu				= new CSteamApps003;
CSteamUserStats006*				g_pSteamUserStatsEmu		= new CSteamUserStats006;

class CSteamClient009 : public ISteamClient008
{
public:

	HSteamPipe CreateSteamPipe()
	{
		Logger(lSAPI, "CSteamClient", "CreateSteamPipe");
		return 0;
	}

	bool ReleaseSteamPipe( HSteamPipe hSteamPipe )
	{
		Logger(lSAPI, "CSteamClient", "ReleaseSteamPipe");
		return true;
	}
	
	HSteamUser CreateGlobalUser( HSteamPipe *phSteamPipe ) {
		Logger(lSAPI, "CSteamClient", "CreateGlobalUser");
		return NULL;
	}

	HSteamUser ConnectToGlobalUser( HSteamPipe hSteamPipe )
	{
		Logger(lSAPI, "CSteamClient", "ConnectToGlobalUser");
		return NULL;
	}

	HSteamUser CreateLocalUser( HSteamPipe *phSteamPipe, EAccountType eAccountType )
	{
		Logger(lSAPI, "CSteamClient", "CreateLocalUser");
		return NULL;
	}

	void ReleaseUser( HSteamPipe hSteamPipe, HSteamUser hUser )
	{
		Logger(lSAPI, "CSteamClient", "ReleaseUser");
	}

	ISteamUser *GetISteamUser( HSteamUser hSteamUser, HSteamPipe hSteamPipe, const char *pchVersion )
	{
		Logger(lSAPI, "CSteamClient", "GetISteamUser");
		return (ISteamUser*)g_pSteamUserEmu;
	}
	
	IVAC *GetIVAC( HSteamUser hSteamUser ) {
		Logger(lSAPI, "ISteamClient", "GetIVAC");
		return NULL;
	}

	ISteamGameServer *GetISteamGameServer( HSteamUser hSteamUser, HSteamPipe hSteamPipe, const char *pchVersion )
	{
		Logger(lSAPI, "CSteamClient", "GetISteamGameServer");
		return (ISteamGameServer *)g_pSteamGameServerEmu;
	}

	void SetLocalIPBinding( uint32 unIP, uint16 usPort )
	{
		Logger(lSAPI, "CSteamClient", "SetLocalIPBinding");
	}

	ISteamFriends *GetISteamFriends( HSteamUser hSteamUser, HSteamPipe hSteamPipe, const char *pchVersion )
	{
		Logger(lSAPI, "CSteamClient", "GetISteamFriends");
		return (ISteamFriends *)g_pSteamFriendsEmu;
	}

	ISteamUtils *GetISteamUtils( HSteamPipe hSteamPipe, const char *pchVersion )
	{
		Logger(lSAPI, "CSteamClient", "GetISteamUtils");
		return (ISteamUtils *)g_pSteamUtilsEmu;
	}

	ISteamMatchmaking *GetISteamMatchmaking( HSteamUser hSteamUser, HSteamPipe hSteamPipe, const char *pchVersion )
	{
		Logger(lSAPI, "CSteamClient", "GetISteamMatchmaking");
		return (ISteamMatchmaking *)g_pSteamMatchmakingEmu;
	}

	ISteamMasterServerUpdater *GetISteamMasterServerUpdater( HSteamUser hSteamUser, HSteamPipe hSteamPipe, const char *pchVersion )
	{
		Logger(lSAPI, "CSteamClient", "GetISteamMasterServerUpdater");
		return (ISteamMasterServerUpdater *)g_pSteamMasterServerUpEmu;
	}

	ISteamMatchmakingServers *GetISteamMatchmakingServers( HSteamUser hSteamUser, HSteamPipe hSteamPipe, const char *pchVersion )
	{
		Logger(lSAPI, "CSteamClient", "GetISteamMatchmakingServers");
		//return (ISteamMatchmakingServers *)g_pSteamMatchmakingServEmu;
		return NULL;
	}

	void *GetISteamGenericInterface( HSteamUser hSteamUser, HSteamPipe hSteamPipe, const char *pchVersion )
	{
		Logger(lSAPI, "CSteamClient", "GetISteamGenericInterface");
		return NULL;
	}

	ISteamUserStats *GetISteamUserStats( HSteamUser hSteamUser, HSteamPipe hSteamPipe, const char *pchVersion )
	{
		Logger(lSAPI, "CSteamClient", "GetISteamUserStats");
		return (ISteamUserStats *)g_pSteamUserStatsEmu;
	}

	ISteamApps *GetISteamApps( HSteamUser hSteamUser, HSteamPipe hSteamPipe, const char *pchVersion )
	{
		Logger(lSAPI, "CSteamClient", "GetISteamApps");
		return (ISteamApps *)g_pSteamAppsEmu;
	}

	ISteamNetworking *GetISteamNetworking( HSteamUser hSteamUser, HSteamPipe hSteamPipe, const char *pchVersion )
	{
		Logger(lSAPI, "CSteamClient", "GetISteamNetworking");
		return (ISteamNetworking *)g_pSteamNetworkingEmu;
	}

	ISteamRemoteStorage *GetISteamRemoteStorage( HSteamUser hSteamuser, HSteamPipe hSteamPipe, const char *pchVersion )
	{
		Logger(lSAPI, "CSteamClient", "GetISteamRemoteStorage");
		return (ISteamRemoteStorage *)g_pSteamRemoteStorageEmu;
	}

	void RunFrame()
	{
		Logger(lSAPI, "CSteamClient", "RunFrame");
		return;
	}

	uint32 GetIPCCallCount()
	{
		Logger(lSAPI, "CSteamClient", "GetIPCCallCount");
		return 0;
	}

	void SetWarningMessageHook( SteamAPIWarningMessageHook_t pFunction )
	{
		Logger(lSAPI, "CSteamClient", "SetWarningMessageHook");
	}
};

CSteamClient009* g_pSteamClientEmu = new CSteamClient009;

/*
========================
NTAuthority's Code Start
========================
*/
// for strings
unsigned int oneAtATimeHash(char* inpStr)
{
	unsigned int value = 0,temp = 0;
	for(size_t i=0;i<strlen(inpStr);i++)
	{
		char ctext = tolower(inpStr[i]);
		temp = ctext;
		temp += value;
		value = temp << 10;
		temp += value;
		value = temp >> 6;
		value ^= temp;
	}
	temp = value << 3;
	temp += value;
	unsigned int temp2 = temp >> 11;
	temp = temp2 ^ temp;
	temp2 = temp << 15;
	value = temp2 + temp;
	if(value < 2) value += 2;
	return value;
}

typedef hostent* (WINAPI *gethostbyname_t)(const char* name);
gethostbyname_t realgethostbyname = (gethostbyname_t)gethostbyname;

char* serverName = NULL;
char* webName = NULL;

hostent* WINAPI custom_gethostbyname(const char* name) {
	// if the name is IWNet's stuff...
	unsigned int ip1 = oneAtATimeHash("ip1.pc.iw4.iwnet.infinityward.com");
	unsigned int log1 = oneAtATimeHash("log1.pc.iw4.iwnet.infinityward.com");
	unsigned int match1 = oneAtATimeHash("match1.pc.iw4.iwnet.infinityward.com");
	unsigned int web1 = oneAtATimeHash("web1.pc.iw4.iwnet.infinityward.com");
	unsigned int blob1 = oneAtATimeHash("blob1.pc.iw4.iwnet.infinityward.com");

	unsigned int current = oneAtATimeHash((char*)name);
	char* hostname = (char*)name;

	if (current == log1 || current == match1 || current == blob1) {
		if (!serverName) {
			serverName = "192.168.1.3";
		}

		hostname = serverName;
	}

	if (current == ip1) {
		if (!serverName) {
			serverName = "192.168.1.3";
		}

		hostname = serverName;
	}

	if (current == web1) {
		if (!webName) {
			webName = "192.168.1.3";
		}

		hostname = webName;
	}

	return realgethostbyname(hostname);
}
/*
======================
NTAuthority's Code End
======================
*/

#pragma managed

extern "C"
{
	__declspec(dllexport) HSteamPipe __cdecl GetHSteamPipe()
	{
		//Logger(lSAPI, "SteamAPI", "GetHSteamPipe");
		return NULL;
	}
	__declspec(dllexport) HSteamUser __cdecl GetHSteamUser()
	{
		//Logger(lSAPI, "SteamAPI", "GetHSteamUser");
		return NULL;
	}
	__declspec(dllexport) HSteamPipe __cdecl SteamAPI_GetHSteamPipe()
	{
		//Logger(lSAPI, "SteamAPI", "SteamAPI_GetHSteamPipe");
		return NULL;
	}
	__declspec(dllexport) HSteamUser __cdecl SteamAPI_GetHSteamUser()
	{
		//Logger(lSAPI, "SteamAPI", "SteamAPI_GetHSteamUser");
		return NULL;
	}
	__declspec(dllexport) const char *__cdecl SteamAPI_GetSteamInstallPath()
	{
		//Logger(lSAPI, "SteamAPI", "SteamAPI_GetSteamInstallPath");
		return g_Directory.GetDirectoryFileA( "" );
	}
	__declspec(dllexport) bool __cdecl SteamAPI_Init()
	{
		//TODO
		//Flux login should be here?! -No

		// Host redirection
		PBYTE offset = (PBYTE)GetProcAddress(GetModuleHandleA("ws2_32.dll"),"gethostbyname");
		realgethostbyname = (gethostbyname_t)DetourFunction(offset, (PBYTE)&custom_gethostbyname);
	
		// needs to be here for 'unban' checking
		GetPlayerSteamID();

		Logger(lSAPI, "SteamAPI", "SteamAPI_Init");
		return true;
	}
	__declspec(dllexport) bool __cdecl SteamAPI_InitSafe()
	{
		//Logger(lSAPI, "SteamAPI", "SteamAPI_InitSafe");
		return true;
	}
	__declspec(dllexport) void __cdecl SteamAPI_RegisterCallResult( CCallbackBase* pResult, SteamAPICall_t APICall )
	{
		//Logger(lSAPI, "SteamAPI", "SteamAPI_RegisterCallResult");
		Callbacks::RegisterResult(pResult, APICall);
	}
	__declspec(dllexport) void __cdecl SteamAPI_RegisterCallback( CCallbackBase *pCallback, int iCallback )
	{
		//Logger(lSAPI, "SteamAPI", "SteamAPI_RegisterCallback");
		Callbacks::Register(pCallback, iCallback);
	}
	__declspec(dllexport) void __cdecl SteamAPI_RunCallbacks()
	{
		//Logger(lSAPI, "SteamAPI", "SteamAPI_RunCallbacks");
		Callbacks::Run();
	}
	__declspec(dllexport) void __cdecl SteamAPI_SetMiniDumpComment( const char *pchMsg )
	{
		//Logger(lSAPI, "SteamAPI", "SteamAPI_SetMiniDumpComment");
	}
	__declspec(dllexport) bool __cdecl SteamAPI_SetTryCatchCallbacks( bool bTryCatchCallbacks )
	{
		//Logger(lSAPI, "SteamAPI", "SteamAPI_SetTryCatchCallbacks");
		return bTryCatchCallbacks;
	}
	__declspec(dllexport) void __cdecl SteamAPI_Shutdown()
	{
		//Logger(lSAPI, "SteamAPI", "SteamAPI_Shutdown");
	}
	__declspec(dllexport) void __cdecl SteamAPI_UnregisterCallResult( CCallbackBase* pResult, SteamAPICall_t APICall )
	{
		//Logger(lSAPI, "SteamAPI", "SteamAPI_UnregisterCallResult");
	}
	__declspec(dllexport) void __cdecl SteamAPI_UnregisterCallback( CCallbackBase *pCallback, int iCallback )
	{
		//Logger(lSAPI, "SteamAPI", "SteamAPI_UnregisterCallback");
		Callbacks::Unregister(pCallback);
	}
	__declspec(dllexport) void __cdecl SteamAPI_WriteMiniDump( uint32 uStructuredExceptionCode, void* pvExceptionInfo, uint32 uBuildID )
	{
		//Logger(lSAPI, "SteamAPI", "SteamAPI_WriteMiniDump");
	}

	__declspec(dllexport) void* __cdecl SteamApps()
	{
		//Logger(lSAPI, "SteamAPI", "SteamApps");
		return g_pSteamAppsEmu;
	}
	__declspec(dllexport) void* __cdecl SteamClient()
	{
		//Logger(lSAPI, "SteamAPI", "SteamClient");
		return g_pSteamClientEmu;
	}
	__declspec(dllexport) void* __cdecl SteamContentServer()
	{
		//Logger(lSAPI, "SteamAPI", "SteamContentServer");
		return NULL;
	}
	__declspec(dllexport) void* __cdecl SteamContentServerUtils()
	{
		//Logger(lSAPI, "SteamAPI", "SteamContentServerUtils");
		return NULL;
	}
	__declspec(dllexport) bool __cdecl SteamContentServer_Init( unsigned int uLocalIP, unsigned short usPort )
	{
		//Logger(lSAPI, "SteamAPI", "SteamContentServer_Init");
		return true;
	}
	__declspec(dllexport) void __cdecl SteamContentServer_RunCallbacks()
	{
		//Logger(lSAPI, "SteamAPI", "SteamContentServer_RunCallbacks");
	}
	__declspec(dllexport) void __cdecl SteamContentServer_Shutdown()
	{
		//Logger(lSAPI, "SteamAPI", "SteamContentServer_Shutdown");
	}
	__declspec(dllexport) void* __cdecl SteamFriends()
	{
		//Logger(lSAPI, "SteamAPI", "SteamFriends");
		return (ISteamFriends*)g_pSteamFriendsEmu;
	}
	__declspec(dllexport) void* __cdecl SteamGameServer()
	{
		//Logger(lSAPI, "SteamAPI", "SteamGameServer");
		return (ISteamGameServer*)g_pSteamGameServerEmu;
	}
	__declspec(dllexport) void* __cdecl SteamGameServerUtils()
	{
		//Logger(lSAPI, "SteamAPI", "SteamGameServerUtils");
		return NULL;
	}
	__declspec(dllexport) bool __cdecl SteamGameServer_BSecure()
	{
		//Logger(lSAPI, "SteamAPI", "SteamGameServer_BSecure");
		return true;
	}
	__declspec(dllexport) HSteamPipe __cdecl SteamGameServer_GetHSteamPipe()
	{
		//Logger(lSAPI, "SteamAPI", "SteamGameServer_GetHSteamPipe");
		return NULL;
	}
	__declspec(dllexport) HSteamUser __cdecl SteamGameServer_GetHSteamUser()
	{
		//Logger(lSAPI, "SteamAPI", "SteamGameServer_GetHSteamUser");
		return NULL;
	}
	__declspec(dllexport) int32 __cdecl SteamGameServer_GetIPCCallCount()
	{
		//Logger(lSAPI, "SteamAPI", "SteamGameServer_GetIPCCallCount");
		return NULL;
	}
	__declspec(dllexport) uint64 __cdecl SteamGameServer_GetSteamID()
	{
		//Logger(lSAPI, "SteamAPI", "SteamGameServer_GetSteamID");
		return NULL;
	}
	__declspec(dllexport) bool __cdecl SteamGameServer_Init( uint32 unIP, uint16 usPort, uint16 usGamePort, EServerMode eServerMode, int nGameAppId, const char *pchGameDir, const char *pchVersionString )
	{
		//Logger(lSAPI, "SteamAPI", "SteamGameServer_Init");
		return true;
	}
	__declspec(dllexport) bool __cdecl SteamGameServer_InitSafe( uint32 unIP, uint16 usPort, uint16 usGamePort, EServerMode eServerMode, int nGameAppId, const char *pchGameDir, const char *pchVersionString, unsigned long dongs )
	{
		//Logger(lSAPI, "SteamAPI", "SteamGameServer_InitSafe");
		return true;
	}
	__declspec(dllexport) void __cdecl SteamGameServer_RunCallbacks()
	{
		//Logger(lSAPI, "SteamAPI", "SteamGameServer_RunCallbacks");
	}
	__declspec(dllexport) void __cdecl SteamGameServer_Shutdown()
	{
		//Logger(lSAPI, "SteamAPI", "SteamGameServer_Shutdown");
	}
	__declspec(dllexport) void* __cdecl SteamMasterServerUpdater()
	{
		//Logger(lSAPI, "SteamAPI", "SteamMasterServerUpdater");
		return (ISteamMasterServerUpdater*)g_pSteamMasterServerUpEmu;
	}
	__declspec(dllexport) void* __cdecl SteamMatchmaking()
	{
		//Logger(lSAPI, "SteamAPI", "SteamMatchmaking");
		return (ISteamMatchmaking*)g_pSteamMatchmakingEmu;
	}
	__declspec(dllexport) void* __cdecl SteamMatchmakingServers()
	{
		//Logger(lSAPI, "SteamAPI", "SteamMatchmakingServers");
		//return (ISteamMatchmakingServers*)g_pSteamMatchmakingServEmu;
		return NULL;
	}
	__declspec(dllexport) void* __cdecl SteamNetworking()
	{
		//Logger(lSAPI, "SteamAPI", "SteamNetworking");
		return (ISteamNetworking*)g_pSteamNetworkingEmu;
	}
	__declspec(dllexport) void* __cdecl SteamRemoteStorage()
	{
		//Logger(lSAPI, "SteamAPI", "SteamRemoteStorage");
		return g_pSteamRemoteStorageEmu;
	}
	__declspec(dllexport) void* __cdecl SteamUser()
	{
		//Logger(lSAPI, "SteamAPI", "SteamUser");
		return (ISteamUser*)g_pSteamUserEmu;
	}
	__declspec(dllexport) void* __cdecl SteamUserStats()
	{
		//Logger(lSAPI, "SteamAPI", "SteamUserStats");
		return (ISteamUserStats*)g_pSteamUserStatsEmu; 
	}
	__declspec(dllexport) void* __cdecl SteamUtils()
	{
		//Logger(lSAPI, "SteamAPI", "SteamUtils");
		return g_pSteamUtilsEmu;
	}
	__declspec(dllexport) HSteamUser __cdecl Steam_GetHSteamUserCurrent()
	{
		//Logger(lSAPI, "SteamAPI", "Steam_GetHSteamUserCurrent");
		return NULL;
	}
	__declspec(dllexport) void __cdecl Steam_RegisterInterfaceFuncs( void *hModule )
	{
		//Logger(lSAPI, "SteamAPI", "Steam_RegisterInterfaceFunc");
	}
	__declspec(dllexport) void __cdecl Steam_RunCallbacks( HSteamPipe hSteamPipe, bool bGameServerCallbacks )
	{
		//Logger(lSAPI, "SteamAPI", "Steam_RunCallback");
	}

	__declspec(dllexport) bool __cdecl SteamAPI_RestartApp( int appid )
	{
		//Logger(lSAPI, "SteamAPI", "SteamAPI_RestartApp");
		return true;
	}

    __declspec(dllexport) bool __cdecl SteamAPI_RestartAppIfNecessary( uint32 unOwnAppID )
	{
		//Logger(lSAPI, "SteamAPI", "SteamAPI_RestartAppIfNecessary");
		return true;
	}

	__declspec(dllexport) bool __cdecl SteamAPI_IsSteamRunning()
	{
		//Logger(lSAPI, "SteamAPI", "SteamAPI_IsSteamRunning");
		return true;
	}

	__declspec(dllexport) void *g_pSteamClientGameServer = NULL;
}

#pragma unmanaged

void AIWPatch();
bool initFlux();

int doInit(void *unused)
{
	unused = 0;
	if (initFlux())
	{
		// Create a console window for Steam_API Callbacks and logging
		AllocConsole() ;
		AttachConsole( GetCurrentProcessId() );
		freopen( "CON", "w", stdout ) ;
		SetConsoleTitle( "alterIWnet - Steam_API Debug Console" );

		// Resize console (max length)
		COORD cordinates = {80, 32766};
		HANDLE handle = GetStdHandle(STD_OUTPUT_HANDLE);
		SetConsoleScreenBufferSize(handle, cordinates);

		// Execute AIW Patches
		AIWPatch();
	}
	return 0;
}

BOOL APIENTRY DllMain( HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved )
{
	DisableThreadLibraryCalls(hModule);
    if( ul_reason_for_call == DLL_PROCESS_ATTACH )
    {
		CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)doInit, NULL, NULL, NULL);
    }
 
    return TRUE;
}
