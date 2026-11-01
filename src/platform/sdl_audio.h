/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef SDL_AUDIO_BACKEND_H
#define SDL_AUDIO_BACKEND_H
#include <stdbool.h>
bool Audio_Startup(void);
void Audio_Pump(void);
void Audio_Shutdown(void);
bool Audio_Active(void);
const char *Audio_Error(void);
#endif
