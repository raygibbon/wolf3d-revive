/* SPDX-License-Identifier: GPL-2.0-only */
/* Active normal-level gameplay port of WOLFSRC/WL_DEF.H. */
#ifndef WL_DEF_H
#define WL_DEF_H
#include "ID_MAP.h"
#define TILEGLOBAL INT32_C(65536)
#define MINDIST INT32_C(0x5800)
#define FOCALLENGTH INT32_C(0x5700)
#define ANGLES 360
#define FINEANGLES 3600
#define MAXTICS 10
#define MAXVIEWWIDTH 304
extern uint16_t viewwidth,viewheight,viewxorigin,viewyorigin;
#define VIEWWIDTH viewwidth
#define VIEWHEIGHT viewheight
#define VIEWX viewxorigin
#define VIEWY viewyorigin
bool NewViewSize(unsigned size);
unsigned ViewSize(void);
bool BeginViewMenu(void);
int HandleViewMenu(uint64_t ticks);
int SimulationTics(void); /* Normal play: 1. Original demo frames: 4. */
typedef struct objstruct {
    int32_t x,y; int angle,tilex,tiley;
    int dir,obclass,areanumber,hitpoints,ticcount,temp1,temp2,viewx,viewheight;
    int32_t speed,distance,transx,transy;
    unsigned flags;
    bool active;
    struct statestruct *state;
    struct objstruct *next;
} objtype;
typedef enum { dr_closed,dr_opening,dr_open,dr_closing } dooraction;
typedef struct { unsigned x,y,lock; bool vertical; dooraction action; uint16_t position; unsigned ticcount; } doorobj_t;
extern objtype player;
extern w3r_byte tilemap[64][64]; /* Original x-major runtime indexing. */
extern doorobj_t doorobjlist[64];
extern unsigned doornum;
extern uint32_t sintable[450];
extern int32_t finetangent[900];
extern int pixelangle[MAXVIEWWIDTH];
int32_t FixedByFrac(int32_t a,uint32_t signed_magnitude);
int32_t WL_FloorShift(int64_t value,unsigned bits);
void BuildTables(void);
void CalcProjection(void);
bool SetupGameLevel(void);
void ShutdownGameLevel(void);
bool TryMove(const objtype *ob);
void ClipMove(objtype *ob,int32_t xmove,int32_t ymove);
void Thrust(int angle,int32_t speed);
void ModernMovement(int turn,int forward,int strafe,int move);
void ControlMovement(int controlx,int controly,bool strafe);
void ResetMovement(void);
void OpenDoor(unsigned door);
void CloseDoor(unsigned door);
void OperateDoor(unsigned door);
void MoveDoors(void);
void Cmd_Use(void);
extern unsigned pwallstate,pwallpos;
extern int pwallx,pwally,pwalldir;
extern bool pushable[64][64];
void PushWall(int x,int y,int dir);
void MovePWalls(void);
void ResetPushwalls(void);
bool DoorSolid(unsigned door);
/* Original difficulty order: baby, easy, medium, hard. */
bool NewGame(unsigned difficulty,unsigned episode);
bool GameStartMap(uint64_t ticks,unsigned map,unsigned difficulty,unsigned seed);
bool GameStartSelected(uint64_t ticks,unsigned episode,unsigned difficulty);
bool GameStart(uint64_t ticks);
bool GameWarp(uint64_t ticks,unsigned floor);
typedef enum { ex_playing,ex_completed,ex_secretlevel,ex_victorious } exit_t;
extern exit_t playstate;
bool GameIntermission(void);
bool VictoryTile(void);
void VictorySpin(void);
void ResetVictory(void);
bool DeathCameraActive(void);
bool StepDeathCamera(void);
void DrawPlayBorder(void);
bool DrawDeathCameraMessage(void);
bool DrawVictory(void);
bool StartEndText(void);
bool StepEndText(bool *done);
void ResetEndText(void);
bool GameVictoryPresentation(void);
/* Read-only death presentation: 0 pending/inactive, 1 turn, 2 fizzle, 3 hold. */
unsigned GameDeathPresentation(uint32_t *seed);
unsigned NextMap(unsigned episode,unsigned map,exit_t exit);
typedef struct { unsigned kill,secret,treasure,seconds,par; int bonus; } levelratio_t;
extern levelratio_t LevelRatios[8];
levelratio_t LevelStats(void);
bool DrawLevelCompleted(const levelratio_t *stats);
bool StartLevelIntermission(const levelratio_t *stats,uint64_t ticks);
bool StepLevelIntermission(uint64_t ticks,bool acknowledge,bool *leave);
bool LevelIntermissionCounting(void);
bool GameStep(uint64_t ticks);
void GameStop(void);
bool GameProbe(void);
/* Public trace result supports deterministic tests without an SDL renderer. */
typedef struct { int32_t x,y; unsigned page,column,height; int side,grid,tile; } rayhit;
bool AsmRefresh(int fineangle,rayhit *hit);
bool ThreeDRefresh(void);
bool ThreeDRefreshPaused(void);
void ScalePost(unsigned x,unsigned width,unsigned height,const w3r_byte *column);
#include "WL_ACTOR.h"
#endif
