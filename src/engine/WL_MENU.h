/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef WL_MENU_H
#define WL_MENU_H
#include <stdbool.h>
#include <stdint.h>
#define BORDCOLOR 0x29
#define BORD2COLOR 0x23
#define DEACTIVE 0x2b
#define BKGDCOLOR 0x2d
#define STRIPE 0x2c
#define TEXTCOLOR 0x17
#define HIGHLIGHT 0x13
/* GOODTIMES layout selected by original VERSION.H: no Read This row. */
enum { newgame, soundmenu, control, loadgame, savegame, changeview, viewscores, backtodemo, quit, MENU_COUNT };
typedef struct { int x, y, amount, curpos, indent; } CP_iteminfo;
typedef struct { int active; const char *string; } CP_itemtype;
extern CP_iteminfo MainItems;
extern const CP_itemtype MainMenu[MENU_COUNT];
void ClearMScreen(void);
void DrawStripes(int y);
void DrawWindow(int x, int y, int w, int h, unsigned char color);
bool DrawMainMenu(void);
bool SetupControlPanel(uint64_t ticks);
/* -2: no action; -1: Escape, otherwise original menu item index. */
int HandleMenu(uint64_t ticks);
bool Message(const char *text);
bool DrawHighScores(void);
#endif
