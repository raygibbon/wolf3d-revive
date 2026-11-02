/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef WL_PRESENT_H
#define WL_PRESENT_H
#include "WL_DEF.h"
#include "WL_FRAME_CLOCK.h"
typedef struct {
    int32_t x,y;double angle;
} wl_pose;
typedef struct {
    int32_t x,y;int shape,rotate,dir,angle,kind;uint64_t identity;double facing;
} wl_present_actor;
typedef struct {
    wl_pose camera;
    wl_present_actor actors[MAXACTORS];unsigned order[MAXACTORS],count;
    statobj_t statics[MAXSTATS];unsigned static_count;
    w3r_byte tiles[64][64];doorobj_t doors[64];unsigned door_count;
    int push_x,push_y,push_dir;unsigned push_state;int32_t push_offset;
    unsigned level,view_size,weapon,weapon_frame;uint32_t simulation_clock;
    unsigned ceiling,death_phase;uint32_t death_cursor;
    bool died,victory,demo;
} wl_present_snapshot;
void WL_PresentReset(void);
void WL_PresentAllocated(unsigned slot);
void WL_PresentCapture(uint64_t tick);
/* Writes only caller-owned presentation storage; snapshots remain immutable. */
bool WL_PresentSample(uint64_t now_ns,wl_present_snapshot *out);
const wl_present_snapshot *WL_PresentCurrent(void);
double WL_PresentAngle(double previous,double current,double alpha);
#endif
