/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef SDL_INPUT_H
#define SDL_INPUT_H
#include <stdbool.h>
#include <SDL3/SDL.h>
unsigned IN_MapSDLScancode(unsigned code);
void IN_SDLKey(unsigned code, bool down);
void IN_SDLFocusLost(void);
void IN_SDLFocusGained(void);
void IN_SDLReset(void);
void IN_SDLDiscardInput(void);
bool IN_SDLFocused(void);
void IN_SDLCapture(SDL_Window *window,bool gameplay);
void IN_SDLMouseMotion(double x);
void IN_SDLPointerMotion(double x,double y);
void IN_SDLMouseButton(unsigned button,bool down);
bool IN_SDLCaptured(void);
const char *IN_SDLCaptureError(void);
#endif
