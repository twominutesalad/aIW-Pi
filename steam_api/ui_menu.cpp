// ====================================================================
// ui_menu.cpp
// 
// Hooks into mw2 to intercept menu translations.
// ====================================================================
#include "StdAfx.h"
#include "hooking.h"
#include "ui_menu.h"

PBYTE   MTadr     = (PBYTE)ADR_MENUTRANS_CAVE;
BYTE    MThook[6] = { 0xe9, 0x00,0x00,0x00,0x00, 0x90 };   // jmp xxxx ; nop
DWORD   MTeax, MTstr;

typedef struct {
	char *str;			// menuid string
	char *trans;		// menu translation
} tMENUTRANS;

// our custom translations
tMENUTRANS	MENU[] = {
	{ "XBOXLIVE_SERVICENAME"    , "alterIWnet"  },
	{ "MENU_INVITE_CAPS"        , "SERVER LIST" },
	{ "MPUI_DESC_PARTY_INVITE"  , "Browse through all servers" },
	{ "MENU_DESC_INVITE_FRIENDS"  , "Browse through all servers" },
	{ "MENU_GAMETYPE  EXE_ALL"  , "" },
	{ "MENU_FILTER_SERVERS"  , "" },
	{ "MENU_NEW_FAVORITE"  , "" },
	{ "MENU_DEL_FAVORITE"  , "" },
	{ "MENU_ADD_TO_FAVORITES"  , "" },
	{ "MENU_SERVER_INFO"  , "" },
	{ 0, 0 } };

// find a MENUID in our list
void MENUtranslation()
{
	int Mno;
	for (Mno=0; MENU[Mno].str!=0; Mno++) {
		if (strcmp((PCHAR)MTstr,MENU[Mno].str)==0) {
			MTeax=(DWORD)MENU[Mno].trans;		// force our translation
			return;
		}
	}
}

void __declspec(naked) MENUcave() 
{
	__asm {
		mov eax,[esp+8]
		mov MTeax, eax			// save original translation
		mov eax, [esp+4]
		mov MTstr, eax			// save menuid str
		pushad
		pushfd
	}
#ifdef MENUDEBUGID
	MTeax=MTstr;				// show menuid as translation
#endif
	MENUtranslation();
	__asm {
		popfd
		popad
		mov  eax, MTeax			
		test eax, eax
		jnz  MENUend
		push ADR_MENUTRANS_END
	MENUend:
		retn
	}
}

void MENUtranslate_init()
{
	MEMsetcave((DWORD)MTadr,(DWORD)MENUcave,MThook,6);
}