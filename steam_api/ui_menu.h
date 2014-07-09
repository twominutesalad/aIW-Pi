#ifndef UI_MENU_H
#define UI_MENU_H

// define this to display the MENUID's  instead of the original translations
//#define MENUDEBUGID

#define ADR_MENUTRANS_CAVE	0x004247d0
#define ADR_MENUTRANS_END	0x004247d6

void    MENUtranslate_init(void);			// place a hook inside mw2 

#endif