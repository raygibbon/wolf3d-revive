/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef SDL_VIDEO_H
#define SDL_VIDEO_H
#include <stdbool.h>

bool VL_Startup(void);
void VL_Shutdown(void);
bool VW_UpdateScreen(void);
bool VL_PollQuit(void);
/* Diagnostic capture of the rendered viewport; game artwork stays local. */
bool VL_Capture(const char *path);
const char *VL_Error(void);
#endif
