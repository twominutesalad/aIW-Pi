/*
================================
Class that manipulates the game
UI
================================
*/

#include "stdafx.h"
#include "Hooking.h"
#include <math.h>
#include "ui_mw2.h"

#pragma unmanaged

StompHook menuFileHook;
DWORD menuFileHookLoc = 0x4A48D0;

typedef void* (__cdecl * LoadAssetOfType_t)(int type, const char* filename);
LoadAssetOfType_t LoadAssetOfType = (LoadAssetOfType_t)0x493FB0;

void Logger(unsigned int lvl, char* caller, char* logline, ...);

void AlterJoinMenu(menuDef_t* menu)
{
	/*FILE* htmlFile = fopen("raw\\pc_join_unranked.html", "w");
	fprintf(htmlFile, "<div style='margin-left: auto; margin-right: auto; margin-top: 50px; border: 3px solid #ff0000; position: relative; width: 640px; height: 480px;'>");

	for (int i = 0; i < menu->numItems; i++)
	{
		itemDef_t* item = menu->items[i];
		float x = floor(menu->items[i]->window.rect[0]);
		float y = floor(menu->items[i]->window.rect[1]);
		float w = floor(menu->items[i]->window.rect[2]);
		float h = floor(menu->items[i]->window.rect[3]);

		if (x < 0)
		{
			x = (640 + x) - w;
		}

		if (y < 0)
		{
			y = (480 + y) - h;
		}

		fprintf(htmlFile, "<div style='position: absolute; left: %fpx; top:%fpx; width: %fpx; height:%fpx; border: 1px solid #000;'>", x, y, w, h);
		if (menu->items[i]->window.name)
		{
			fprintf(htmlFile, "%s", menu->items[i]->window.name);
		}
		fprintf(htmlFile, "</div>");
	}

	fprintf(htmlFile, "</div>");
	fclose(htmlFile);*/
	
	for (int i = 0; i < menu->numItems; i++)
	{
		itemDef_t* item = menu->items[i];

		//Logger("pc_join_unranked", "%s", item->mouse_click);

		if (item->window.rect2[1] == 2 || item->window.rect2[1] == 22 || item->window.rect2[1] == 42)
		{
			if (item->window.rect2[1] == 42)
			{
				item->window.flags2 &= 0xFFFFFFFB;
			}

			if (item->window.rect2[0] < 50)
			{
				item->window.flags2 &= 0xFFFFFFFB;
			}

			item->window.rect2[1] -= 36;
		}

		if (item->window.rect2[1] == -26)
		{
			item->window.rect2[1] += 31;

			if (!item->window.name || stricmp(item->window.name, "back") != 0)
			{
				item->window.flags2 &= 0xFFFFFFFB;
			}

			item->window.ownerDrawFlags = 0;
		}

		if (item->type == 6)
		{
			listBoxDef_t* idata = (listBoxDef_t*)item->typeData;

			//idata->columnInfo[2].maxChars += 22;
			idata->columnInfo[2].maxChars = 45;
			
			for (int j = 0; j < idata->numColumns; j++)
			{
				columnInfo_t* column = &idata->columnInfo[j];

				//printf("wutwut");
			}
		}

		if (!item->window.name) continue;

		if (stricmp(item->window.name, "refreshSource") == 0)
		{
			//item->window.name = "updateGay";
			//item->window.rect[0] -= 100;
			//item->window.rect[1] += 50;
			//item->window.rect2[0] -= 100;
			//item->window.rect2[1] -= 32;
			//item->window.flags2 ^= 4;
			//printf("YYYY\n");
		}
	}

	return;
}

typedef int (__cdecl * FS_ReadFile_t)(const char* qpath, void** buffer);
extern FS_ReadFile_t FS_ReadFile;

typedef void (__cdecl * MenuParse_ItemDef_t)(menuDef_t*, int);
MenuParse_ItemDef_t MenuParse_ItemDef = (MenuParse_ItemDef_t)0x6421A0;

void* MenuFileHookFunc(const char* filename)
{
	menuFile_t* file = (menuFile_t*)LoadAssetOfType(0x19, filename);

	// check for any 'pc_join_unranked' menus
	for (int i = 0; i < file->count; i++)
	{
		menuDef_t* menu = file->menuFiles[i];

		//Logger("alterIWnet", "Menu (%s)", menu->window.name);

		if (stricmp(menu->window.name, "pc_join_unranked") == 0)
		{
			AlterJoinMenu(menu);
		}

		/*if (stricmp(menu->window.name, "menu_tickertest") == 0)
		{
			menu->items[0]->window.rect[1] = 445;
			menu->items[1]->window.rect[1] = 445;
			menu->items[2]->window.rect[1] = 445;
			menu->items[0]->window.rect2[1] = 445;
			menu->items[1]->window.rect2[1] = 445;
			menu->items[2]->window.rect2[1] = 445;
		}*/

		//if (stricmp(menu->window.name, "pc_join_unranked") == 0)
		if (stricmp(menu->window.name, "menu_xboxlive") == 0)
		//if (stricmp(menu->window.name, "menu_tickertest") == 0)
		{
			//menuDef_t* newMenu = malloc(sizeof(menuDef_t) + )
			itemDef_t** newItems = (itemDef_t**)malloc(sizeof(itemDef_t*) * (menu->numItems + 10));
			memcpy(newItems, menu->items, sizeof(itemDef_t*) * menu->numItems);
			menu->items = newItems;

			int oldCount = menu->numItems;

			/*for (int i = 0; i < menu->numItems; i++)
			{
				itemDef_t* item = menu->items[i];

				printf("who");
			}*/

			// time to open up the book of unknown magic, and attempt loading an itemDef from the file system
			// NOTE BEFORE RE-ENABLING: THIS CRASHES GAME IN HASHASSETNAMEKEY ON MAP LOAD
			/*if (FS_ReadFile("ui/ntapwns.item", 0) > 0)
			{
				int handle = PC_LoadSource("ui/ntapwns.item");

				if (handle)
				{
					// BLACK MAGIC, EXECUTE.
					//MenuParse_ItemDef(menu, handle);
					Menu_Parse(handle, menu);
					PC_FreeSource(handle);

					if (menu->numItems != oldCount)
					{
						OutputDebugStringA("I guess it worked.\n");
					}
				}
			}*/
		}

		if (strnicmp(menu->window.name, "xpbar", 5) == 0)
		{
			/*menuExpression_t* expression = (menuExpression_t*)menu->visibleExp;
			expression->count -= 2;

			for (int i = 1; i < expression->count; i++)
			{
				expression->tokens[i] = expression->tokens[i + 2];
			}*/

			//menu->visibleExp = NULL;
		}
	}

	return file;
}

void __declspec(naked) MenuFileHookStub()
{
	__asm jmp MenuFileHookFunc
}

void OoeyInit()
{
	// doesn't belong here, but still putting it here -- realloc 'image' asset limit
	*(DWORD*)0x799618 = 0xF00;
	*(DWORD*)0x7998D8 = (DWORD)malloc(0xF00 * 120);

	// this does
	menuFileHook.initialize("aaaaa", 5, (PBYTE)menuFileHookLoc);
	menuFileHook.installHook(MenuFileHookStub, true, true);
}