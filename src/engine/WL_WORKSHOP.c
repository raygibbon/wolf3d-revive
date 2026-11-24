/* SPDX-License-Identifier: GPL-2.0-only */
/* Experimental first-person Workshop state. The live tilemap remains the
   sole renderer/collision authority; this module owns only transient UI state. */
#include "WL_WORKSHOP.h"
#include "ID_US_1.h"
#include "ID_VH.h"
#include "ID_PM.h"
#include "WL_ACTOR.h"
#include "WL_RENDER.h"
#include <stdio.h>
#include <string.h>

#define WORKSHOP_RANGE (TILEGLOBAL*3)
#define WORKSHOP_RAY_STEP (TILEGLOBAL/64)
#define WORKSHOP_PANEL_MARGIN 4
#define WORKSHOP_CROSSHAIR_RADIUS 5
#define WORKSHOP_CROSSHAIR_BOX 16
#define WORKSHOP_BACKUP_HEIGHT 64

static bool enabled;
static unsigned wall_id=1;
static wl_workshop_request pending;
static wl_workshop_target current;
static wl_edit_result feedback=WL_EDIT_OK;
static unsigned feedback_tics;

/* The overlay is transient viewport presentation. Keep a bounded copy of the
   pixels it covers so an explicit clear can restore the framebuffer even when
   no full-screen redraw has happened yet. */
static bool overlay_drawn;
static int overlay_x,overlay_y,overlay_w,overlay_h;
static bool overlay_backup_valid;
static w3r_byte overlay_backup[320*WORKSHOP_BACKUP_HEIGHT];
static uint64_t overlay_serial;
static int crosshair_x,crosshair_y,crosshair_w,crosshair_h;
static bool crosshair_backup_valid;
static w3r_byte crosshair_backup[WORKSHOP_CROSSHAIR_BOX*WORKSHOP_CROSSHAIR_BOX];

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

static void SaveRect(w3r_byte *dest,size_t dest_stride,const w3r_byte *src,size_t src_stride,
                     int x,int y,int width,int height)
{
    for(int row=0;row<height;++row)
        memcpy(dest+(size_t)row*dest_stride,src+(size_t)(y+row)*src_stride+(size_t)x,(size_t)width);
}

static void RestoreRect(w3r_byte *dest,size_t dest_stride,const w3r_byte *src,size_t src_stride,
                        int x,int y,int width,int height)
{
    for(int row=0;row<height;++row)
        memcpy(dest+(size_t)(y+row)*dest_stride+(size_t)x,src+(size_t)row*src_stride,(size_t)width);
}

void WL_WorkshopClearOverlay(void)
{
    if(!overlay_drawn) return;
    /* A redraw after the overlay was captured has already replaced these
       pixels; restoring the saved copy would reintroduce stale scene data. */
    if(WL_RenderSerial()==overlay_serial) {
        if(overlay_backup_valid)
            RestoreRect(framebuffer,320,overlay_backup,320,overlay_x,overlay_y,overlay_w,overlay_h);
        if(crosshair_backup_valid)
            RestoreRect(framebuffer,320,crosshair_backup,WORKSHOP_CROSSHAIR_BOX,
                        crosshair_x,crosshair_y,crosshair_w,crosshair_h);
    }
    overlay_drawn=false;overlay_backup_valid=false;crosshair_backup_valid=false;
}

void WL_WorkshopReset(void)
{ WL_WorkshopClearOverlay();enabled=false;pending=WL_WORKSHOP_NONE;memset(&current,0,sizeof(current));feedback=WL_EDIT_OK;feedback_tics=0;wall_id=1; }
bool WL_WorkshopEnabled(void) { return enabled; }
unsigned WL_WorkshopWall(void) { return wall_id; }

void WL_WorkshopSetEnabled(bool value)
{
    WL_WorkshopClearOverlay();
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

void WL_WorkshopTick(void)
{
    if(!enabled) return;
    if(feedback_tics) --feedback_tics;
    WL_WorkshopUpdateTarget();
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

static bool PreviewValid(void)
{ return current.hit && (current.place_result==WL_EDIT_OK || current.remove_result==WL_EDIT_OK); }

static void FitOverlayLine(char *line,size_t size,int max_width)
{
    if(!line || !size) return;
    int width=0,height=0;
    if(!VW_MeasurePropString(line,&width,&height)) return;
    while(*line && width>max_width) {
        size_t length=strlen(line);
        if(length<=1) { line[0]=0;break; }
        line[length-1]=0;
        if(!VW_MeasurePropString(line,&width,&height)) break;
    }
}

static void DrawClassicCrosshair(int cx,int cy,w3r_byte color)
{
    VWB_Hlin(cx-WORKSHOP_CROSSHAIR_RADIUS,cx-2,cy,color);
    VWB_Hlin(cx+2,cx+WORKSHOP_CROSSHAIR_RADIUS,cy,color);
    VWB_Vlin(cy-WORKSHOP_CROSSHAIR_RADIUS,cy-2,cx,color);
    VWB_Vlin(cy+2,cy+WORKSHOP_CROSSHAIR_RADIUS,cx,color);
}

void WL_WorkshopDrawClassicOverlay(void)
{
    WL_WorkshopClearOverlay();
    if(!enabled) return;

    int old_font=fontnumber,old_px=px,old_py=py,old_print_x=PrintX,old_print_y=PrintY,old_window=WindowX;
    w3r_byte old_color=fontcolor;
    fontnumber=0;

    int line_h=8,measured=0;
    if(!VW_MeasurePropString("W",&measured,&line_h)) line_h=8;

    int panel_w=(int)VIEWWIDTH-WORKSHOP_PANEL_MARGIN*2;
    if(panel_w>304) panel_w=304;
    if(panel_w<0) panel_w=0;
    int panel_h=line_h*2+4;
    int max_h=(int)VIEWHEIGHT-WORKSHOP_PANEL_MARGIN;
    if(panel_h>max_h) panel_h=max_h;
    if(panel_h<0) panel_h=0;
    int panel_x=(int)VIEWX+((int)VIEWWIDTH-panel_w)/2;
    int panel_y=(int)VIEWY+(int)VIEWHEIGHT-panel_h-WORKSHOP_PANEL_MARGIN/2;

    overlay_x=panel_x;overlay_y=panel_y;overlay_w=panel_w;overlay_h=panel_h;
    if(panel_w>0 && panel_h>0 && panel_h<=WORKSHOP_BACKUP_HEIGHT) {
        SaveRect(overlay_backup,320,framebuffer,320,panel_x,panel_y,panel_w,panel_h);
        overlay_backup_valid=true;
    }

    int cx=(int)VIEWX+(int)VIEWWIDTH/2,cy=(int)VIEWY+(int)VIEWHEIGHT/2;
    crosshair_x=cx-WORKSHOP_CROSSHAIR_BOX/2;crosshair_y=cy-WORKSHOP_CROSSHAIR_BOX/2;
    crosshair_w=crosshair_h=WORKSHOP_CROSSHAIR_BOX;
    SaveRect(crosshair_backup,WORKSHOP_CROSSHAIR_BOX,framebuffer,320,
             crosshair_x,crosshair_y,crosshair_w,crosshair_h);
    crosshair_backup_valid=true;

    if(panel_w>0 && panel_h>0) {
        VWB_Bar(panel_x,panel_y,panel_w,panel_h,0x7f);
        char line[96];
        fontcolor=0x0f;
        PrintX=WindowX=panel_x+4;PrintY=panel_y+2;
        (void)snprintf(line,sizeof(line),"WORKSHOP  FIRE=PLACE  USE=REMOVE");
        FitOverlayLine(line,sizeof(line),panel_w-8);
        (void)US_Print(line);

        int second_y=panel_y+2+line_h;
        if(second_y+line_h<=(int)VIEWY+(int)VIEWHEIGHT) {
            PrintX=WindowX=panel_x+4;PrintY=second_y;
            if(feedback_tics) {
                fontcolor=feedback==WL_EDIT_OK?0x4a:0x4c;
                (void)snprintf(line,sizeof(line),"W%u  EDIT %s  `=EXIT",wall_id,WL_EditResultName(feedback));
            } else if(current.hit) {
                fontcolor=PreviewValid()?0x4a:0x4c;
                (void)snprintf(line,sizeof(line),"W%u P%d,%d %s R%d,%d %s  `=EXIT",wall_id,
                    current.place_x,current.place_y,WL_EditResultName(current.place_result),
                    current.remove_x,current.remove_y,WL_EditResultName(current.remove_result));
            } else {
                fontcolor=0x4c;
                (void)snprintf(line,sizeof(line),"W%u  NO TARGET WITHIN 3 TILES  `=EXIT",wall_id);
            }
            FitOverlayLine(line,sizeof(line),panel_w-8);
            (void)US_Print(line);
        }
    }

    DrawClassicCrosshair(cx,cy,PreviewValid()?0x4a:0x4c);

    fontnumber=old_font;fontcolor=old_color;px=old_px;py=old_py;PrintX=old_print_x;PrintY=old_print_y;WindowX=old_window;
    overlay_serial=WL_RenderSerial();
    overlay_drawn=true;
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

static void CopyClassicPanel(wl_render_target *target,int x,int y,int width,int height)
{
    if(!overlay_drawn || overlay_w<=0 || overlay_h<=0 || width<1 || height<1) return;
    if(WL_RenderSerial()!=overlay_serial || !VIEWWIDTH || !VIEWHEIGHT) return;
    int dx=x+(overlay_x-(int)VIEWX)*width/(int)VIEWWIDTH;
    int dy=y+(overlay_y-(int)VIEWY)*height/(int)VIEWHEIGHT;
    int dw=overlay_w*width/(int)VIEWWIDTH;
    int dh=overlay_h*height/(int)VIEWHEIGHT;
    if(dw<=0 || dh<=0) return;
    for(int row=0;row<dh;++row) {
        int sy=overlay_y+row*overlay_h/dh;
        int ty=dy+row;
        if(ty<0 || ty>=(int)target->height) continue;
        for(int col=0;col<dw;++col) {
            int sx=overlay_x+col*overlay_w/dw;
            int tx=dx+col;
            if(tx<0 || tx>=(int)target->width) continue;
            target->pixels[(size_t)ty*target->stride+(unsigned)tx]=framebuffer[(size_t)sy*320+(unsigned)sx];
        }
    }
}

void WL_WorkshopDrawTargetOverlay(wl_render_target *target,int x,int y,int width,int height)
{
    if(!enabled || !overlay_drawn || !target || width<1 || height<1) return;
    if(WL_RenderSerial()!=overlay_serial) return;
    CopyClassicPanel(target,x,y,width,height);
    w3r_byte color=PreviewValid()?0x4a:0x4c;
    int cx=x+width/2,cy=y+height/2,arm=width/64,thick=width/320;
    if(arm<4) arm=4;
    if(thick<1) thick=1;
    TargetBar(target,cx-arm,cy,arm-thick*2,thick,color);
    TargetBar(target,cx+thick*2,cy,arm-thick*2,thick,color);
    TargetBar(target,cx,cy-arm,thick,arm-thick*2,color);
    TargetBar(target,cx,cy+thick*2,thick,arm-thick*2,color);
}
