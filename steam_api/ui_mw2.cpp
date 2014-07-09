#include "stdafx.h"
#include "ui_mw2.h"

#pragma unmanaged
typedef script_t * (__cdecl * LoadScriptFile_t)(const char*);
typedef int (__cdecl * PC_ReadToken_t)(source_t*, token_t*);
typedef void (__cdecl * Com_Printf_t)(int, const char*, ...);

// 1.0.182 function definitions
LoadScriptFile_t LoadScriptFile = (LoadScriptFile_t)0x4D99C0;
PC_ReadToken_t PC_ReadToken = (PC_ReadToken_t)0x46F270;
Com_Printf_t CCom_Printf = (Com_Printf_t)0x45DAC0;

source_t **sourceFiles = (source_t **)0x7C60F8;
keywordHash_t **menuParseKeywordHash = (keywordHash_t **)0x6406428;

// 'shared' code
int KeywordHash_Key(char *keyword) {
	int register hash, i;

	hash = 0;
	for (i = 0; keyword[i] != '\0'; i++) {
		if (keyword[i] >= 'A' && keyword[i] <= 'Z')
			hash += (keyword[i] + ('a' - 'A')) * (119 + i);
		else
			hash += keyword[i] * (119 + i);
	}
	hash = (hash ^ (hash >> 10) ^ (hash >> 20)) & (KEYWORDHASH_SIZE-1);
	return hash;
}

/*keywordHash_t *KeywordHash_Find(keywordHash_t *table[], char *keyword)
{
	keywordHash_t *key;
	int hash;

	hash = KeywordHash_Key(keyword);
	for (key = table[hash]; key; key = key->next) {
		if (!stricmp(key->keyword, keyword))
			return key;
	}
	return NULL;
}*/

void FreeScript(script_t *script)
{
#ifdef PUNCTABLE
	if (script->punctuationtable) FreeMemory(script->punctuationtable);
#endif //PUNCTABLE
	free(script);
}

void StripDoubleQuotes(char *string)
{
	if (*string == '\"')
	{
		strcpy(string, string+1);
	} //end if
	if (string[strlen(string)-1] == '\"')
	{
		string[strlen(string)-1] = '\0';
	} //end if
} 

source_t *LoadSourceFile(const char *filename)
{
	source_t *source;
	script_t *script;

	script = LoadScriptFile(filename);
	if (!script) return NULL;

	script->next = NULL;

	source = (source_t *) malloc(sizeof(source_t));
	memset(source, 0, sizeof(source_t));

	strncpy(source->filename, filename, 64);
	source->scriptstack = script;
	source->tokens = NULL;
	source->defines = NULL;
	source->indentstack = NULL;
	source->skip = 0;
	source->definehash = (define_t**)malloc(4096);
	memset(source->definehash, 0, 4096);

	return source;
} 

void FreeSource(source_t *source)
{
	script_t *script;
	token_t *token;
	define_t *define;
	indent_t *indent;
	int i;

	//PC_PrintDefineHashTable(source->definehash);
	//free all the scripts
	while(source->scriptstack)
	{
		script = source->scriptstack;
		source->scriptstack = source->scriptstack->next;
		FreeScript(script);
	} //end for
	//free all the tokens
	while(source->tokens)
	{
		token = source->tokens;
		source->tokens = source->tokens->next;
		//PC_FreeToken(token);
	} //end for
#if DEFINEHASHING
	for (i = 0; i < DEFINEHASHSIZE; i++)
	{
		while(source->definehash[i])
		{
			define = source->definehash[i];
			source->definehash[i] = source->definehash[i]->hashnext;
			PC_FreeDefine(define);
		} //end while
	} //end for
#else //DEFINEHASHING
	//free all defines
	while(source->defines)
	{
		define = source->defines;
		source->defines = source->defines->next;
		//PC_FreeDefine(define);
	} //end for
#endif //DEFINEHASHING
	//free all indents
	while(source->indentstack)
	{
		indent = source->indentstack;
		source->indentstack = source->indentstack->next;
		free(indent);
	} //end for
#if DEFINEHASHING
	//
	if (source->definehash) free(source->definehash);
#endif //DEFINEHASHING
	//free the source itself
	free(source);
}

#define MAX_SOURCEFILES		64

int PC_LoadSource(const char *filename)
{
	source_t *source;
	int i;

	for (i = 1; i < MAX_SOURCEFILES; i++)
	{
		if (!sourceFiles[i])
			break;
	} //end for
	if (i >= MAX_SOURCEFILES)
		return 0;
	//PS_SetBaseFolder("");
	source = LoadSourceFile(filename);
	if (!source)
		return 0;
	sourceFiles[i] = source;
	return i;
}

int PC_FreeSource(int handle)
{
	if (handle < 1 || handle >= MAX_SOURCEFILES)
		return false;
	if (!sourceFiles[handle])
		return false;

	FreeSource(sourceFiles[handle]);
	sourceFiles[handle] = NULL;
	return true;
}

int PC_ReadTokenHandle(int handle, pc_token_t *pc_token)
{
	token_t token;
	int ret;

	if (handle < 1 || handle >= MAX_SOURCEFILES)
		return 0;
	if (!sourceFiles[handle])
		return 0;

	ret = PC_ReadToken(sourceFiles[handle], &token);
	strcpy(pc_token->string, token.string);
	pc_token->type = token.type;
	pc_token->subtype = token.subtype;
	pc_token->intvalue = token.intvalue;
	pc_token->floatvalue = token.floatvalue;
	if (pc_token->type == TT_STRING)
		StripDoubleQuotes(pc_token->string);
	return ret;
}

// UI code
/*void Item_SetScreenCoords(itemDef_t *item, float x, float y) {
  
  if (item == NULL) {
    return;
  }

  if (item->window.border != 0) {
    x += item->window.borderSize;
    y += item->window.borderSize;
  }

  item->window.rect.x = x + item->window.rectClient.x;
  item->window.rect.y = y + item->window.rectClient.y;
  item->window.rect.w = item->window.rectClient.w;
  item->window.rect.h = item->window.rectClient.h;

  // force the text rects to recompute
  item->textRect.w = 0;
  item->textRect.h = 0;
}

void Menu_UpdatePosition(menuDef_t *menu) {
  int i;
  float x, y;

  if (menu == NULL) {
    return;
  }
  
  x = menu->window.rect.x;
  y = menu->window.rect.y;
  if (menu->window.border != 0) {
    x += menu->window.borderSize;
    y += menu->window.borderSize;
  }

  for (i = 0; i < menu->itemCount; i++) {
    Item_SetScreenCoords(menu->items[i], x, y);
  }
}*/

void Menu_PostParse(menuDef_t *menu) {
	return;
	/*if (menu == NULL) {
		return;
	}
	if (menu->fullScreen) {
		menu->window.rect.x = 0;
		menu->window.rect.y = 0;
		menu->window.rect.w = 640;
		menu->window.rect.h = 480;
	}
	Menu_UpdatePosition(menu);*/
}

void Menu_Init(menuDef_t *menu) {
	memset(menu, 0, 8192);
	//Window_Init(&menu->window);
}

int KeywordHash_Data(char* key)
{
	// patch this function on-the-fly, as it's some ugly C.
	DWORD func = 0x640C80;
	int retval = 0;

	*(DWORD*)0x640C8E = 3523;
	*(DWORD*)0x640CBB = 0x7F;

	__asm
	{
		mov eax, key
		call func
		mov retval, eax
	}

	*(DWORD*)0x640C8E = 531;
	*(DWORD*)0x640CBB = 0x1FF;

	return retval;
}

bool Menu_Parse(int handle, menuDef_t *menu) {
	pc_token_t token;
	keywordHash_t *key;

	if (!PC_ReadTokenHandle(handle, &token))
		return false;
	if (*token.string != '{') {
		return false;
	}
    
	while ( 1 ) {

		memset(&token, 0, sizeof(pc_token_t));
		if (!PC_ReadTokenHandle(handle, &token)) {
			//PC_SourceError(handle, "end of file inside menu\n");
			return false;
		}

		if (*token.string == '}') {
			return true;
		}

		//key = KeywordHash_Find(menuParseKeywordHash, token.string);
		int idx = KeywordHash_Data(token.string);
		key = menuParseKeywordHash[idx];
		if (!key) {
			//PC_SourceError(handle, "unknown menu keyword %s", token.string);
			continue;
		}
		if ( !key->func((itemDef_t*)menu, handle) ) {
			//PC_SourceError(handle, "couldn't parse menu keyword %s", token.string);
			return false;
		}
	}
	return false; 	// bk001205 - LCC missing return value
}

menuDef_t* Menu_New(int handle) {
	menuDef_t *menu = (menuDef_t*)malloc(8192); // FIXME: tentative size

	Menu_Init(menu);
	if (Menu_Parse(handle, menu)) {
		Menu_PostParse(menu);
	}

	return menu;
}

menuDef_t* UI_ParseMenu(const char *menuFile) {
	int handle;
	pc_token_t token;
	menuDef_t* retval = NULL;

	CCom_Printf(0, "Parsing menu file:%s\n", menuFile);

	handle = PC_LoadSource(menuFile);
	if (!handle) {
		return NULL;
	}

	while ( 1 ) {
		memset(&token, 0, sizeof(pc_token_t));
		if (!PC_ReadTokenHandle( handle, &token )) {
			break;
		}

		if ( token.string[0] == '}' ) {
			break;
		}

		if (stricmp(token.string, "menudef") == 0) {
			// start a new menu
			retval = Menu_New(handle);
		}
	}
	
	PC_FreeSource(handle);

	return retval;
}