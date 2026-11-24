/* SPDX-License-Identifier: GPL-2.0-only */
/* Experimental first-person Workshop state. The live tilemap remains the
   sole renderer/collision authority; this module owns only transient UI state. */
#include "WL_WORKSHOP.h"
#include "ID_US_1.h"
#include "ID_VH.h"
#include "WL_LEVEL.h"
#include "ID_IN.h"
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

/* Material/hotbar state is headless-testable and independent of SDL. */
static unsigned materials[64];
static unsigned material_count;
static unsigned hotbar[9];
static unsigned selected_slot;
static unsigned material_page;
static bool picker_open;

/* Message buffer for workshop status. */
static char message[64];
static bool message_error;
static unsigned message_tics;

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
    int free_x=-1,free_y=-1;
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
        if(free_x<0) { free_x=cell_x;free_y=cell_y; }
        last_x=cell_x;last_y=cell_y;
    }
    /* If no surface was hit, use the first empty grid cell actually crossed by
       the ray. This works from every position within the source tile; a fixed
       half-tile sample does not reliably cross a boundary. */
    if(free_x<0) return false;
    target->hit=false;target->has_place=true;
    target->place_x=free_x;target->place_y=free_y;
    target->place_result=WL_EditCanSetWall(world,free_x,free_y,selected);
    return true;
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
unsigned WL_WorkshopWall(void) { return WL_WorkshopHotbarMaterial(selected_slot); }

void WL_WorkshopSetEnabled(bool value)
{
    WL_WorkshopClearOverlay();
    enabled=value;pending=WL_WORKSHOP_NONE;feedback=WL_EDIT_OK;feedback_tics=0;
    if(enabled) {
        WL_WorkshopSaveStatusBar();
        /* Reuse a wall already validated by level setup when possible. */
        for(unsigned y=1;y<63;++y) for(unsigned x=1;x<63;++x) {
            unsigned tile=tilemap[x][y];
            if(tile>0 && tile<=63 && tile!=21 && tile!=22) { wall_id=tile;goto selected; }
        }
selected:
        WL_WorkshopRefreshMaterials();
        WL_WorkshopUpdateTarget();
    } else {
        WL_WorkshopRestoreStatusBar();
        memset(&current,0,sizeof(current));
    }
}

void WL_WorkshopRequest(wl_workshop_request request)
{ if(enabled && pending==WL_WORKSHOP_NONE && request!=WL_WORKSHOP_NONE) pending=request; }

void WL_WorkshopUpdateTarget(void)
{
    if(!enabled) { memset(&current,0,sizeof(current));return; }
    wl_edit_world world=WL_WorkshopLiveWorld();
    (void)WL_WorkshopSelect(&world,player.x,player.y,player.angle,WL_WorkshopWall(),&current);
}

void WL_WorkshopTick(void)
{
    if(!enabled) return;
    if(feedback_tics) --feedback_tics;
    /* Number keys assign from the visible picker page, or select a hotbar
       slot when the picker is closed. Consume each edge exactly once. */
    for(unsigned slot=0;slot<WL_WORKSHOP_SLOTS;++slot) if(IN_ConsumePress(0x02U+slot)) {
        if(picker_open) (void)WL_WorkshopPickerAssign(slot);
        else (void)WL_WorkshopSelectSlot(slot);
        break;
    }
    /* Toggle material picker with E key (only in Workshop mode) */
    if(IN_ConsumePress(0x13)) { WL_WorkshopPickerToggle(); }
    WL_WorkshopUpdateTarget();
}

wl_edit_result WL_WorkshopCommit(void)
{
    if(!enabled || pending==WL_WORKSHOP_NONE) return WL_EDIT_OK;
    WL_WorkshopUpdateTarget();
    wl_edit_world world=WL_WorkshopLiveWorld();
    wl_edit_result result=WL_EDIT_INVALID_STATE;
    if(pending==WL_WORKSHOP_PLACE && current.has_place)
        result=WL_EditSetWall(&world,current.place_x,current.place_y,WL_WorkshopWall());
    else if(pending==WL_WORKSHOP_REMOVE && current.has_remove)
        result=WL_EditRemoveWall(&world,current.remove_x,current.remove_y);
    pending=WL_WORKSHOP_NONE;feedback=result;feedback_tics=70;WL_WorkshopUpdateTarget();
    return result;
}

const wl_workshop_target *WL_WorkshopTarget(void) { return &current; }

static bool PreviewValid(void)
{ return (current.has_place && current.place_result==WL_EDIT_OK) ||
         (current.has_remove && current.remove_result==WL_EDIT_OK); }

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

static void DrawMaterialPreview(int x,int y,int size,unsigned id)
{
    size_t length=0;
    const w3r_byte *texture=PM_GetPage((id-1U)*2U,&length);
    if(!texture || length<4096U) {
        for(int jy=0;jy<size;++jy) for(int jx=0;jx<size;++jx)
            framebuffer[(size_t)(y+jy)*320+(size_t)(x+jx)]=(w3r_byte)(((jx/4+jy/4)&1)?0x2d:0x23);
        return;
    }
    for(int jy=0;jy<size;++jy) for(int jx=0;jx<size;++jx) {
        unsigned sx=(unsigned)(jx*64/size),sy=(unsigned)(jy*64/size);
        framebuffer[(size_t)(y+jy)*320+(size_t)(x+jx)]=texture[(size_t)sy*64U+sx];
    }
}

static void DrawSlot(int x,int y,unsigned number,unsigned id,bool selected)
{
    VWB_Bar(x,y,32,18,0x2d);
    w3r_byte border=selected?0x13:0x23;
    VWB_Hlin(x,x+31,y,border);VWB_Hlin(x,x+31,y+17,border);
    VWB_Vlin(y,y+17,x,border);VWB_Vlin(y,y+17,x+31,border);
    char label[2]={(char)('1'+(int)number),0};
    fontcolor=selected?0x0f:0x17;PrintX=x+2;PrintY=y+5;(void)US_Print(label);
    if(id) DrawMaterialPreview(x+13,y+1,16,id);
    else { fontcolor=0x17;PrintX=x+8;PrintY=y+5;(void)US_Print("-"); }
}

void WL_WorkshopRefreshMaterials(void)
{
    unsigned ids[64],count=0;
    unsigned limit=PMSpriteStart/2U;
    if(limit>63U) limit=63U;
    for(unsigned id=1;id<=limit;++id) {
        if(!WL_LevelMaterialValid(id)) continue;
        if(!PM_PagePresent((id-1U)*2U) || !PM_PagePresent((id-1U)*2U+1U)) continue;
        ids[count++]=id;
    }
    (void)WL_WorkshopSetMaterials(ids,count);
}

bool WL_WorkshopSetMaterials(const unsigned *ids,unsigned count)
{
    if(count>64 || (count && !ids)) return false;
    bool used[64]={false};
    for(unsigned i=0;i<count;++i) {
        unsigned id=ids[i];
        if(id>63U || !WL_LevelMaterialValid(id) || used[id]) return false;
        used[id]=true;
    }
    memset(materials,0,sizeof(materials));
    for(unsigned i=0;i<count;++i) materials[i]=ids[i];
    material_count=count;
    memset(hotbar,0,sizeof(hotbar));
    for(unsigned i=0;i<count && i<9;++i) hotbar[i]=materials[i];
    selected_slot=0;material_page=0;picker_open=false;
    if(enabled) WL_WorkshopUpdateTarget();
    return true;
}

unsigned WL_WorkshopMaterialCount(void) { return material_count; }
unsigned WL_WorkshopMaterialAt(unsigned index) { return index<material_count?materials[index]:0U; }
unsigned WL_WorkshopFirstMaterial(void) { return material_count?materials[0]:0U; }
unsigned WL_WorkshopHotbarMaterial(unsigned slot) { return slot<9?hotbar[slot]:0U; }
unsigned WL_WorkshopSelectedSlot(void) { return selected_slot; }

bool WL_WorkshopMaterialAvailable(unsigned id)
{
    for(unsigned i=0;i<material_count;++i) if(materials[i]==id) return true;
    return false;
}

bool WL_WorkshopSelectSlot(unsigned slot)
{
    if(slot>=9 || !hotbar[slot]) return false;
    selected_slot=slot;
    if(enabled) WL_WorkshopUpdateTarget();
    return true;
}

bool WL_WorkshopSetSlotMaterial(unsigned slot,unsigned id)
{
    if(slot>=9 || !WL_WorkshopMaterialAvailable(id)) return false;
    hotbar[slot]=id;
    if(enabled) WL_WorkshopUpdateTarget();
    return true;
}

void WL_WorkshopCycle(int direction)
{
    if(!direction) return;
    for(unsigned step=1;step<=9;++step) {
        unsigned slot;
        if(direction>0) slot=(selected_slot+step)%9;
        else slot=(selected_slot+9-(step%9))%9;
        if(hotbar[slot]) { (void)WL_WorkshopSelectSlot(slot);return; }
    }
}

bool WL_WorkshopPickerOpen(void) { return picker_open; }
unsigned WL_WorkshopPickerPage(void) { return material_page; }
unsigned WL_WorkshopPickerPageCount(void) { return (material_count+8U)/9; }

void WL_WorkshopPickerToggle(void)
{
    if(picker_open) { picker_open=false;return; }
    unsigned wall=WL_WorkshopWall();
    material_page=0;
    for(unsigned i=0;i<material_count;++i) if(materials[i]==wall) { material_page=i/9;break; }
    picker_open=true;
}

bool WL_WorkshopPickerAssign(unsigned page_index)
{
    if(!picker_open || page_index>=9) return false;
    unsigned index=material_page*9+page_index;
    if(index>=material_count) return false;
    hotbar[selected_slot]=materials[index];
    picker_open=false;
    if(enabled) WL_WorkshopUpdateTarget();
    return true;
}

void WL_WorkshopMessage(const char *text,bool error)
{
    if(!text) text="";
    (void)snprintf(message,sizeof(message),"%s",text);
    message_error=error;
    message_tics=140;
}

/* Status bar backup for HUD restoration. */
static w3r_byte status_bar_backup[320*40];
static bool status_bar_backup_valid;

/* Workshop status bar replaces the normal bottom status bar. */
void WL_WorkshopSaveStatusBar(void)
{
    if(status_bar_backup_valid) return;
    memcpy(status_bar_backup,framebuffer+160*320,sizeof(status_bar_backup));
    status_bar_backup_valid=true;
}

void WL_WorkshopRestoreStatusBar(void)
{
    if(status_bar_backup_valid) {
        memcpy(framebuffer+160*320,status_bar_backup,sizeof(status_bar_backup));
        status_bar_backup_valid=false;
    } else {
        VWB_Bar(0,160,320,40,127);
    }
}

/* Status bar drawing draws the nine-slot hotbar in the bottom status bar area.
   When Workshop is enabled, this replaces the normal HUD. On exit, ClearOverlay
   restores the saved HUD pixels. */
void WL_WorkshopDrawStatusBar(void)
{
    if(!enabled) return;

    int old_font=fontnumber,old_px=px,old_py=py,old_print_x=PrintX,old_print_y=PrintY,old_window=WindowX;
    w3r_byte old_color=fontcolor;
    fontnumber=0;WindowX=0;

    VWB_Bar(0,160,320,40,0x2d);
    VWB_Hlin(0,319,160,0x23);VWB_Hlin(0,319,199,0x23);

    unsigned wall=WL_WorkshopWall();
    const char *status="NO TARGET";
    if(message_tics) { status=message; }
    else if(feedback_tics) { status=WL_EditResultName(feedback); }
    else if(picker_open) { status="PICK MATERIAL"; }
    else if(current.has_place && current.place_result==WL_EDIT_OK) { status="PLACE READY"; }
    else if(current.has_remove && current.remove_result==WL_EDIT_OK) { status="REMOVE READY"; }
    else if(current.has_place) { status=WL_EditResultName(current.place_result); }
    else if(current.has_remove) { status=WL_EditResultName(current.remove_result); }

    char line[96];
    if(picker_open) {
        (void)snprintf(line,sizeof(line),"WORKSHOP  ASSIGN SLOT %u  %s",selected_slot+1U,status);
    } else {
        if(wall) (void)snprintf(line,sizeof(line),"WORKSHOP  WALL %u  %s",wall,status);
        else (void)snprintf(line,sizeof(line),"WORKSHOP  NO MATERIAL  %s",status);
    }
    fontcolor=0x17;PrintX=4;PrintY=162;(void)US_Print(line);

    if(picker_open) {
        (void)snprintf(line,sizeof(line),"PICK  PAGE %u/%u  E CLOSE  1-9 ASSIGN",material_page+1U,WL_WorkshopPickerPageCount()?WL_WorkshopPickerPageCount():1U);
    } else {
        (void)snprintf(line,sizeof(line),"1-9 SLOT  E PICK  WHEEL  F5 SAVE  L LOAD  ` EXIT");
    }
    fontcolor=0x17;PrintX=4;PrintY=171;(void)US_Print(line);

    for(unsigned i=0;i<WL_WORKSHOP_SLOTS;++i) {
        unsigned id;
        bool selected;
        if(picker_open) {
            unsigned index=material_page*WL_WORKSHOP_SLOTS+i;
            id=index<material_count?materials[index]:0U;
            selected=id && id==wall;
        } else {
            id=hotbar[i];
            selected=i==selected_slot;
        }
        DrawSlot(7+(int)i*34,181,i,id,selected);
    }

    fontnumber=old_font;fontcolor=old_color;px=old_px;py=old_py;
    PrintX=old_print_x;PrintY=old_print_y;WindowX=old_window;
}
