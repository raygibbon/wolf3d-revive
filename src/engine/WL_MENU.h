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
extern CP_itemtype MainMenu[MENU_COUNT];
int MenuCursorPicture(void);
int MenuNavigation(void); /* Up/down edges and original 20-tick held repeat. */
void MenuCursorMoved(void);
void ResetMenuCursor(uint64_t ticks);
bool TickMenuCursor(uint64_t ticks);
void ClearMScreen(void);
void DrawStripes(int y);
void DrawWindow(int x, int y, int w, int h, unsigned char color);
bool DrawMainMenu(void);
bool SetupControlPanel(uint64_t ticks);
/* -2: no action; -1: Escape, otherwise original menu item index. */
int HandleMenu(uint64_t ticks);
bool DrawNewSelection(bool episode,unsigned choice);
int HandleNewSelection(uint64_t ticks,bool episode,unsigned *choice);
bool BeginControlsMenu(void);
bool ResumeControlsMenu(void);
bool DrawControlsMenu(void);
int HandleControlsMenu(void);
bool DrawSaveActivity(bool saving,bool second);
bool BeginSaveMenu(bool saving);
int HandleSaveMenu(uint64_t ticks);
unsigned SaveMenuSelected(void);
bool DrawSoundSettings(void);
bool BeginSoundSettings(void);
int HandleSoundSettings(void);
bool Message(const char *text);
bool MessageAtHeight(const char *text,unsigned height);
/* Separate UI entropy: DOS quit selection must never advance gameplay RNG. */
const char *MenuQuitMessage(unsigned first,unsigned second);
bool BeginConfirmation(const char *text,uint64_t ticks);
bool BeginConfirmationAtHeight(const char *text,uint64_t ticks,unsigned height);
/* -3 error, -1 cancelled, 0 pending, 1 accepted; waits for answer release. */
int HandleConfirmation(uint64_t ticks);
bool DrawHighScores(void);
#endif
