/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef WL_MAIN_H
#define WL_MAIN_H
#include <stdbool.h>
#include <stdint.h>
typedef enum { UI_TITLE, UI_CREDITS, UI_HIGHSCORES, UI_MENU, UI_SCORES, UI_CONFIRM, UI_GAME, UI_EPISODE, UI_DIFFICULTY, UI_ERROR, UI_SOUND, UI_SAVE, UI_LOAD, UI_CONTROLS, UI_DISPLAY, UI_VIEW, UI_BOSS, UI_DEMO, UI_QUICK, UI_DONE, UI_RATING, UI_SIGNON } ui_state;
bool DemoLoopStartup(uint64_t ticks, bool menu);
/* Optional verified archive PG13 picture; normal title/menu API stays unchanged. */
bool DemoLoopIntro(uint64_t ticks);
/* Optional user-supplied 320x200 indexed sign-on image. Missing file is safe. */
bool DemoLoopSignon(const char *path,uint64_t ticks);
bool DemoLoop(uint64_t ticks);
ui_state DemoLoopState(void);
void DemoLoopPrepareExit(void);
bool DemoLoopStartGame(uint64_t ticks);
#endif
