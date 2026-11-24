/* SPDX-License-Identifier: GPL-2.0-only */
/* Small live-world mutation boundary used by the experimental Workshop. */
#ifndef WL_WORLD_EDIT_H
#define WL_WORLD_EDIT_H
#include "WL_DEF.h"

typedef enum {
    WL_EDIT_OK = 0,
    WL_EDIT_OUT_OF_BOUNDS,
    WL_EDIT_INVALID_WALL,
    WL_EDIT_OCCUPIED,
    WL_EDIT_PROTECTED,
    WL_EDIT_NOT_WALL,
    WL_EDIT_INVALID_STATE
} wl_edit_result;

typedef struct {
    w3r_byte (*tiles)[64];
    objtype *(*actors)[64];
    bool (*blocked_statics)[64];
    bool (*pushable)[64];
    const statobj_t *statics;
    unsigned static_count;
    const objtype *player;
    unsigned width,height,wall_limit;
    unsigned push_state;
    int push_x,push_y;
} wl_edit_world;

bool WL_EditWorldValid(const wl_edit_world *world);
wl_edit_result WL_EditCanSetWall(const wl_edit_world *world,int x,int y,unsigned wall_id);
wl_edit_result WL_EditCanRemoveWall(const wl_edit_world *world,int x,int y);
wl_edit_result WL_EditSetWall(wl_edit_world *world,int x,int y,unsigned wall_id);
wl_edit_result WL_EditRemoveWall(wl_edit_world *world,int x,int y);
bool WL_EditTileBlocked(const wl_edit_world *world,int x,int y);
const char *WL_EditResultName(wl_edit_result result);

#endif
