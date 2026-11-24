/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef WL_WORKSHOP_H
#define WL_WORKSHOP_H
#include "WL_WORLD_EDIT.h"
#include "WL_RENDER_TARGET.h"

typedef struct {
    bool hit,has_place,has_remove;
    int place_x,place_y,remove_x,remove_y;
    wl_edit_result place_result,remove_result;
} wl_workshop_target;

typedef enum { WL_WORKSHOP_NONE,WL_WORKSHOP_PLACE,WL_WORKSHOP_REMOVE } wl_workshop_request;

wl_edit_world WL_WorkshopLiveWorld(void);
bool WL_WorkshopSelect(const wl_edit_world *world,int32_t x,int32_t y,int angle,
                       unsigned wall_id,wl_workshop_target *target);
void WL_WorkshopReset(void);
bool WL_WorkshopEnabled(void);
void WL_WorkshopSetEnabled(bool enabled);
void WL_WorkshopRequest(wl_workshop_request request);
wl_edit_result WL_WorkshopCommit(void);
void WL_WorkshopUpdateTarget(void);
void WL_WorkshopTick(void);
void WL_WorkshopClearOverlay(void);
const wl_workshop_target *WL_WorkshopTarget(void);
unsigned WL_WorkshopWall(void);
void WL_WorkshopDrawClassicOverlay(void);
void WL_WorkshopDrawTargetOverlay(wl_render_target *target,int x,int y,int width,int height);

#endif
