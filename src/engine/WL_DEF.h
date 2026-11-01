/* SPDX-License-Identifier: GPL-2.0-only */
/* Active wall/movement slice of WOLFSRC/WL_DEF.H. */
#ifndef WL_DEF_H
#define WL_DEF_H
#include "ID_MAP.h"
#define TILEGLOBAL INT32_C(65536)
#define MINDIST INT32_C(0x5800)
#define FOCALLENGTH INT32_C(0x5700)
#define ANGLES 360
#define FINEANGLES 3600
#define MAXTICS 10
#define VIEWWIDTH 240
#define VIEWHEIGHT 120
#define VIEWX 40
#define VIEWY 20
typedef struct { int32_t x,y; int angle; unsigned tilex,tiley; } objtype;
typedef enum { dr_closed,dr_opening,dr_open,dr_closing } dooraction;
typedef struct { unsigned x,y,lock; bool vertical; dooraction action; uint16_t position; unsigned ticcount; } doorobj_t;
extern objtype player;
extern w3r_byte tilemap[64][64]; /* Original x-major runtime indexing. */
extern doorobj_t doorobjlist[64];
extern unsigned doornum;
extern uint32_t sintable[450];
extern int32_t finetangent[900];
extern int pixelangle[VIEWWIDTH];
int32_t FixedByFrac(int32_t a,uint32_t signed_magnitude);
int32_t WL_FloorShift(int64_t value,unsigned bits);
void BuildTables(void);
void CalcProjection(void);
bool SetupGameLevel(void);
void ShutdownGameLevel(void);
bool TryMove(const objtype *ob);
void ClipMove(objtype *ob,int32_t xmove,int32_t ymove);
void Thrust(int angle,int32_t speed);
void ControlMovement(int controlx,int controly,bool strafe);
void ResetMovement(void);
void OpenDoor(unsigned door);
void CloseDoor(unsigned door);
void OperateDoor(unsigned door);
void MoveDoors(void);
void Cmd_Use(void);
bool DoorSolid(unsigned door);
bool GameStart(uint64_t ticks);
bool GameStep(uint64_t ticks);
void GameStop(void);
bool GameProbe(void);
/* Public trace result supports deterministic tests without an SDL renderer. */
typedef struct { int32_t x,y; unsigned page,column,height; int side,grid,tile; } rayhit;
bool AsmRefresh(int fineangle,rayhit *hit);
bool ThreeDRefresh(void);
void ScalePost(unsigned x,unsigned width,unsigned height,const w3r_byte *column);
#endif
