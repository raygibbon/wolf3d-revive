/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef ID_SD_H
#define ID_SD_H
#include "WL_SOUND.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
/* All APIs run on the main thread. SDL consumes copied PCM, never this state. */
bool SD_Startup(unsigned rate);
void SD_Shutdown(void);
bool SD_Ready(void);
/* Logical original mode, independent of SDL availability and mute. Configure
   before a level starts; front end selects digitized + AdLib by default. */
void SD_SetDigitizedMode(bool enabled);
bool SD_DigitizedMode(void);
void SD_StartMusic(int track); /* -1 stops music; original indices 0..26. */
int SD_LevelMusic(unsigned episode,unsigned floor);
bool SD_IsDigitizedSound(soundnames sound);
bool SD_PlaySound(soundnames sound,unsigned left,unsigned right);
void SD_StopSound(void);
void SD_FlushOutput(void);
uint64_t SD_OutputGeneration(void);
void SD_SetPosition(unsigned left,unsigned right);
void SD_Render(int16_t *stereo,size_t frames);
void SD_SetVolumes(unsigned music,unsigned effects); /* 0..100 */
void SD_SetEnabled(bool music,bool effects);
void SD_GetSettings(unsigned *music,unsigned *effects,bool *music_on,bool *effects_on);
/* Read-only diagnostics also permit scheduling/priority regressions. */
typedef struct {
    uint64_t frames,ticks,loops,writes;
    int music,sound,digitized;
    unsigned sound_priority,digi_priority,left,right;
    uint8_t registers[256];
} sd_status;
void SD_GetStatus(sd_status *status);
/* Original logical AL wait in 70-Hz presentation tics, independent of device. */
unsigned SD_SoundTicks(soundnames sound);
#endif
