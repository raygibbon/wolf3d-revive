/* SPDX-License-Identifier: GPL-2.0-only */
/* Set-1 keyboard state from WOLFSRC/ID_IN.H/C, without DOS IRQ handling. */
#ifndef ID_IN_H
#define ID_IN_H
#include <stdbool.h>
#include <stdint.h>
enum { sc_None = 0, sc_Escape = 1, sc_Enter = 0x1c, sc_Space = 0x39,
       sc_UpArrow = 0x48, sc_DownArrow = 0x50, sc_LeftArrow = 0x4b, sc_RightArrow = 0x4d,
       sc_Y = 0x15, sc_N = 0x31 };
extern bool Keyboard[128];
extern uint8_t LastScan;
extern char LastASCII;
void IN_ClearKeysDown(void);
void IN_ClearPressed(void);
void IN_KeyEvent(unsigned scan, bool down);
void IN_MenuKeyEvent(unsigned scan,bool down);
bool IN_ConsumePress(unsigned scan);
bool IN_AnyPressed(void);
void IN_AckEvent(void); /* Fresh mouse press outside relative gameplay. */
char IN_ScanASCII(unsigned scan);
char IN_NextASCII(void);
bool IN_PressPending(unsigned scan);
#endif
