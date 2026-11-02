/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef WL_SAVE_H
#define WL_SAVE_H
#include "WL_DEF.h"
#define SAVE_SLOTS 6
#define SAVE_DESCRIPTION 32
#define SAVE_VERSION 1U
typedef enum { SAVE_EMPTY,SAVE_VALID,SAVE_CORRUPT,SAVE_UNSUPPORTED,SAVE_UNAVAILABLE } save_status;
typedef struct { save_status status;char description[SAVE_DESCRIPTION];unsigned episode,level,difficulty,version; } save_info;
bool Save_SetDirectory(const char *directory);
const char *Save_Directory(void);
void Save_ReadInfo(unsigned slot,save_info *info);
bool Save_Write(unsigned slot,const char *description,bool overwrite);
bool Save_Load(unsigned slot,uint64_t ticks);
/* Internal snapshot boundaries. These structs are NEVER written as C bytes. */
typedef struct { int facecount,anglefrac;bool attackheld,gotgatgun; } save_controls;
void Save_GetCombat(save_controls *controls);
void Save_SetCombat(const save_controls *controls);
int Save_GetAngleFraction(void);
void Save_SetAngleFraction(int fraction);
bool Save_ActorAllocated(unsigned slot);
void Save_SetActorAllocation(const bool used[MAXACTORS]);
bool GameCanSave(void);
bool GameRunning(void);
void GameRestoreClock(uint64_t ticks);
void GamePauseClock(uint64_t ticks);
bool GameDrawPaused(void);
/* Stable registry: append identifiers; changing existing entries requires a
   format version bump. Zero means no state, never a process address. */
unsigned Save_StateID(const statetype *state);
statetype *Save_State(unsigned id);
unsigned Save_StateCount(void);
#endif
