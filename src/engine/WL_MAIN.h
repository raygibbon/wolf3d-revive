/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef WL_MAIN_H
#define WL_MAIN_H
#include <stdbool.h>
#include <stdint.h>
#include "game_definition.h"
#include "WL_LEVEL.h"
typedef enum { UI_TITLE, UI_CREDITS, UI_HIGHSCORES, UI_MENU, UI_SCORES, UI_CONFIRM, UI_GAME, UI_EPISODE, UI_DIFFICULTY, UI_ERROR, UI_SOUND, UI_SAVE, UI_LOAD, UI_CONTROLS, UI_DISPLAY, UI_VIEW, UI_BOSS, UI_DEMO, UI_QUICK, UI_DONE, UI_RATING, UI_SIGNON, UI_PAUSE, UI_RESULTS, UI_MESSAGE, UI_JUKEBOX, UI_DISK, UI_WARP } ui_state;
/* Runtime equivalents of DOS capability lights; no invented EMS/XMS counts. */
void DemoLoopStartupDevices(bool mouse,bool synthesis,bool digitized);
bool DemoLoopJukebox(uint64_t ticks,unsigned bank);
void DemoLoopAllowDebug(bool allow);
bool GameGodMode(void);
bool DemoLoopStartup(uint64_t ticks, bool menu);
/* Optional verified archive PG13 picture; normal title/menu API stays unchanged. */
bool DemoLoopIntro(uint64_t ticks);
/* Optional headerless 320x200 row-major image using gamepal. Call after
   Startup/Intro for normal startup only; missing art leaves that state intact.
   Malformed/unreadable art fails before replacing the framebuffer. */
bool DemoLoopSignon(const char *path,uint64_t ticks);
bool DemoLoop(uint64_t ticks);
ui_state DemoLoopState(void);
void DemoLoopPrepareExit(void);
bool DemoLoopStartGame(uint64_t ticks);
bool GameInstallAuthoredLevel(const wl_level *level);
bool GameAuthoredLevel(void);
void GameClearAuthoredLevel(void);
bool GameCaptureAuthoredLevel(wl_level *level);
bool GameSaveAuthoredLevel(const char *directory,char *error,size_t error_size);
bool GameLoadAuthoredLevel(const char *directory,char *error,size_t error_size);
/* Select/clear the borrowed game definition used by menu and jukebox
   consumers. NULL (or an invalid definition) restores the compiled-in
   Wolf3D default. A non-default definition must stay alive until replaced
   or cleared; call WL_SetGameDefinition(NULL) during shutdown. */
void WL_SetGameDefinition(const game_definition_t *def);
const game_definition_t *WL_GameDefinition(void);
/* Legacy 3-bank x 6-slot jukebox adapter. Returns NULL for invalid bank/slot
   or when the selected definition has no name at that fixed position. */
const char *JukeboxTrackName(const game_definition_t *def, unsigned bank, unsigned slot);
#endif
