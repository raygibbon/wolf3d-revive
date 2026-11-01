/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef SDL_INPUT_H
#define SDL_INPUT_H
#include <stdbool.h>
unsigned IN_MapSDLScancode(unsigned code);
void IN_SDLKey(unsigned code, bool down);
void IN_SDLFocusLost(void);
#endif
