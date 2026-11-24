/* SPDX-License-Identifier: GPL-2.0-only */
/* Experimental first-person Workshop state. The live tilemap remains the
   sole renderer/collision authority; this module owns only transient UI state. */
#include "WL_WORKSHOP.h"
#include "ID_US_1.h"
#include "ID_VH.h"
#include "ID_PM.h"
#include "WL_ACTOR.h"
#include <stdio.h>
#include <string.h>

#define WORKSHOP_RANGE (TILEGLOBAL*3)
#define WORKSHOP_RAY_STEP (TILEGLOBAL/64)

static bool enabled;
static unsigned wall_id=1;
static wl_workshop_request pending;
static wl_workshop_target current;
static wl_edit_result feedback=WL_EDIT_OK;
static unsigned feedback_tics;

wl_edit_world WL_WorkshopLiveWorld(void)
{
    unsigned wall_limit=PMSpriteStart/2;
    if(wall_limit>63) wall_limit=63;
    return (wl_edit_world){tilemap,actorat,staticblock,pushable,statobjlist,statcount,&player,
        64,64,wall_limit,pwallstate,pwallx,pwally};
}

static int TileCoordinate(int32_t value) { return WL_FloorShift(value,16); }

bool WL_WorkshopSelect(const wl_edit_world *world,int32_t x,int32_t y,int angle,
                       unsigned selected,wl_workshop_target *target)
{
    if(target) memset(target,0,sizeof(*target));
    if(!target || !WL_EditWorldValid(world) || angle<0 || angle>=360) return false;
    int last_x=TileCoordinate(x),last_y=TileCoordinate(y);
    if(last_x<0 || last_y<0 || (unsigned)last_x>=world->width || (unsigned)last_y>=world->height) return false;
    for(int32_t distance=WORKSHOP_RAY_STEP;distance<=WORKSHOP_RANGE;distance+=WORKSHOP_RAY_STEP) {
        int32_t sample_x=x+FixedByFrac(distance,sintable[angle+90]);
        int32_t sample_y=y-FixedByFrac(distance,sintable[angle]);
        int cell_x=TileCoordinate(sample_x),cell_y=TileCoordinate(sample_y);
        if(cell_x==last_x && cell_y==last_y) continue;
        if(cell_x<0 || cell_y<0 || (unsigned)cell_x>=world->width || (unsigned)cell_y>=world->height) return false;
        unsigned tile=world->tiles[cell_x][cell_y];
        if(tile) {
            target->hit=true;target->has_place=true;target->has_remove=true;
            target->place_x=last_x;target->place_y=last_y;
            target->remove_x=cell_x;target->remove_y=cell_y;
            target->place_result=WL_EditCanSetWall(world,last_x,last_y,selected);
            target->remove_result=WL_EditCanRemoveWall(world,cell_x,cell_y);
            return true;
        }
        last_x=cell_x;last_y=cell_y;
    }
    return false;
}

void WL_WorkshopReset(void)
{ enabled=false;pending=WL_WORKSHOP_NONE;memset(&current,0,sizeof(current));feedback=WL_EDIT_OK;feedback_tics=0;wall_id=1; }
bool WL_WorkshopEnabled(void) { return enabled; }
unsigned WL_WorkshopWall(void) { return wall_id; }

void WL_WorkshopSetEnabled(bool value)
{
    enabled=value;pending=WL_WORKSHOP_NONE;feedback=WL_EDIT_OK;feedback_tics=0;
    if(enabled) {
        /* Reuse a wall already validated by level setup when possible. */
        for(unsigned y=1;y<63;++y) for(unsigned x=1;x<63;++x) {
            unsigned tile=tilemap[x][y];
            if(tile>0 && tile<=63 && tile!=21 && tile!=22) { wall_id=tile;goto selected; }
        }
selected:
        WL_WorkshopUpdateTarget();
    } else memset(&current,0,sizeof(current));
}

void WL_WorkshopRequest(wl_workshop_request request)
{ if(enabled && pending==WL_WORKSHOP_NONE && request!=WL_WORKSHOP_NONE) pending=request; }

void WL_WorkshopUpdateTarget(void)
{
    if(!enabled) { memset(&current,0,sizeof(current));return; }
    wl_edit_world world=WL_WorkshopLiveWorld();
    (void)WL_WorkshopSelect(&world,player.x,player.y,player.angle,wall_id,&current);
}

wl_edit_result WL_WorkshopCommit(void)
{
    if(!enabled || pending==WL_WORKSHOP_NONE) return WL_EDIT_OK;
    WL_WorkshopUpdateTarget();
    wl_edit_world world=WL_WorkshopLiveWorld();
    wl_edit_result result=WL_EDIT_INVALID_STATE;
    if(pending==WL_WORKSHOP_PLACE && current.has_place)
        result=WL_EditSetWall(&world,current.place_x,current.place_y,wall_id);
    else if(pending==WL_WORKSHOP_REMOVE && current.has_remove)
        result=WL_EditRemoveWall(&world,current.remove_x,current.remove_y);
    pending=WL_WORKSHOP_NONE;feedback=result;feedback_tics=70;WL_WorkshopUpdateTarget();
    return result;
}

const wl_workshop_target *WL_WorkshopTarget(void) { return &current; }

static void ClassicCrosshair(w3r_byte color)
{
    int cx=(int)VIEWX+(int)VIEWWIDTH/2,cy=(int)VIEWY+(int)VIEWHEIGHT/2;
    VWB_Hlin(cx-5,cx-2,cy,color);VWB_Hlin(cx+2,cx+5,cy,color);
    VWB_Vlin(cy-5,cy-2,cx,color);VWB_Vlin(cy+2,cy+5,cx,color);
}

static bool PreviewValid(void)
{ return current.hit && (current.place_result==WL_EDIT_OK || current.remove_result==WL_EDIT_OK); }

void WL_WorkshopDrawClassicOverlay(void)
{
    if(!enabled) return;
    ClassicCrosshair(PreviewValid()?0x4a:0x4c);
    VWB_Bar(0,160,320,24,0x7f);
    int old_font=fontnumber,old_px=px,old_py=py,old_print_x=PrintX,old_print_y=PrintY,old_window=WindowX;
    w3r_byte old_color=fontcolor;char line[96];
    fontnumber=0;fontcolor=0x0f;PrintX=4;PrintY=162;WindowX=4;
    (void)snprintf(line,sizeof(line),"WORKSHOP W%u  FIRE=PLACE USE=REMOVE  `=EXIT",wall_id);(void)US_Print(line);
    PrintX=4;PrintY=173;
    if(feedback_tics) {
        fontcolor=feedback==WL_EDIT_OK?0x4a:0x4c;
        (void)snprintf(line,sizeof(line),"EDIT %s",WL_EditResultName(feedback));--feedback_tics;
    } else if(current.hit) {
        fontcolor=PreviewValid()?0x4a:0x4c;
        (void)snprintf(line,sizeof(line),"P %d,%d %s  R %d,%d %s",current.place_x,current.place_y,
            WL_EditResultName(current.place_result),current.remove_x,current.remove_y,WL_EditResultName(current.remove_result));
    } else { fontcolor=0x4c;(void)snprintf(line,sizeof(line),"NO TARGET WITHIN 3 TILES"); }
    (void)US_Print(line);
    fontnumber=old_font;fontcolor=old_color;px=old_px;py=old_py;PrintX=old_print_x;PrintY=old_print_y;WindowX=old_window;
}

static void TargetBar(wl_render_target *target,int x,int y,int width,int height,w3r_byte color)
{
    if(!target || width<=0 || height<=0) return;
    for(int row=0;row<height;++row) {
        int row_y=y+row;if(row_y<0 || (unsigned)row_y>=target->height) continue;
        int start=x<0?0:x,end=x+width>(int)target->width?(int)target->width:x+width;
        if(start<end) memset(target->pixels+(size_t)row_y*target->stride+(unsigned)start,color,(size_t)(end-start));
    }
}

void WL_WorkshopDrawTargetOverlay(wl_render_target *target,int x,int y,int width,int height)
{
    if(!enabled || !target || width<1 || height<1) return;
    w3r_byte color=PreviewValid()?0x4a:0x4c;
    int cx=x+width/2,cy=y+height/2,arm=width/64,thick=width/320;
    if(arm<4) arm=4;
    if(thick<1) thick=1;
    TargetBar(target,cx-arm,cy,arm-thick*2,thick,color);
    TargetBar(target,cx+thick*2,cy,arm-thick*2,thick,color);
    TargetBar(target,cx,cy-arm,thick,arm-thick*2,color);
    TargetBar(target,cx,cy+thick*2,thick,arm-thick*2,color);
}
