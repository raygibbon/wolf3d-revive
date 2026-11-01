/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef WL_MAIN_H
#define WL_MAIN_H
#include <stdbool.h>
#include <stdint.h>
typedef enum { UI_TITLE, UI_CREDITS, UI_HIGHSCORES, UI_MENU, UI_SCORES, UI_CONFIRM, UI_DONE } ui_state;
bool DemoLoopStartup(uint64_t ticks, bool menu);
bool DemoLoop(uint64_t ticks);
ui_state DemoLoopState(void);
#endif
