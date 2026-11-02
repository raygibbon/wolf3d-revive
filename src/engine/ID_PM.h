/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef ID_PM_H
#define ID_PM_H
#include "w3r_types.h"
#include <stdbool.h>
#include <stddef.h>
extern unsigned ChunksInFile,PMSpriteStart,PMSoundStart;
bool PM_Startup(void);
bool PM_Ready(void);
void PM_Shutdown(void);
const w3r_byte *PM_GetPage(unsigned page,size_t *length);
bool PM_DecodeSprite(unsigned sprite,w3r_byte pixels[4096],w3r_byte opaque[4096]);
bool PM_PagePresent(unsigned page);
#endif
