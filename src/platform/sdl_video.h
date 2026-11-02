/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef SDL_VIDEO_H
#define SDL_VIDEO_H
#include <stdbool.h>
#include <stdint.h>
#include "display_settings.h"

bool VL_Startup(void);
bool VL_DisplayGet(display_settings *settings);
bool VL_DisplayPreferences(display_settings *settings); /* Pending accepted intent for persistence. */
bool VL_DisplaySet(const display_settings *settings);
#ifdef WOLF3D_DISPLAY_TEST
/* One-shot fault injection, compiled only into the display test executable. */
uint64_t VL_TestSceneUploads(void);
void VL_TestDisplayFailure(unsigned operation); /* 1 fullscreen, 2 scaling, 3 size, 4 denied request, 5 allocation, 6 texture, 7 VSync, 8 filter */
void VL_TestExpireDisplayRequest(void);
#endif
uint64_t VL_GetTicks70(void);
unsigned VL_MonitorFrameRate(void); /* Current output refresh, bounded, 60 FPS fallback. */
void VL_Sleep(unsigned ms);
uint64_t VL_TextureUploads(void);
void VL_Shutdown(void);
bool VW_UpdateScreen(void);
bool VL_PollQuit(void);
/* Diagnostic capture of the rendered viewport; game artwork stays local. */
bool VL_Screenshot(void);
bool VL_Capture(const char *path);
const char *VL_Error(void);
#endif
