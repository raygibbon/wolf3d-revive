/* SPDX-License-Identifier: GPL-2.0-only */
#include "WL_DISPLAY.h"
#include "WL_MENU.h"
#include "ID_IN.h"
#include "ID_CA.h"
#include "ID_VH.h"
#include "ID_US_1.h"
#include <stdio.h>
#include <string.h>
static bool (*Get)(display_settings *),(*Set)(const display_settings *);
static unsigned selected;
static display_settings previous;
/* Window sizing is independent of the 4:3 DOS menu canvas and 3D render size. */
typedef struct { int width,height; } window_preset;
static const window_preset window_presets[]={
    {640,480},{960,720},{1280,960},
    {960,600},{1280,800},{1600,1000},{1920,1200},
    {960,540},{1280,720},{1600,900},{1920,1080},
    {1680,720},{2560,1080},
    {1920,540},{2560,720}
};
#define WINDOW_PRESET_COUNT (sizeof(window_presets)/sizeof(window_presets[0]))
static void CycleWindowSize(display_settings *s,bool backwards)
{
    size_t current=WINDOW_PRESET_COUNT;
    for(size_t i=0;i<WINDOW_PRESET_COUNT;++i)
        if(s->width==window_presets[i].width && s->height==window_presets[i].height) { current=i;break; }
    size_t next=current==WINDOW_PRESET_COUNT?(backwards?WINDOW_PRESET_COUNT-1:0):
        (current+(backwards?WINDOW_PRESET_COUNT-1:1))%WINDOW_PRESET_COUNT;
    s->scale=0;s->width=window_presets[next].width;s->height=window_presets[next].height;
}
void Display_Connect(bool (*get)(display_settings *),bool (*set)(const display_settings *)) { Get=get;Set=set; }
bool Display_Available(void) { return Get && Set; }
static bool Text(int x,int y,const char *s) { PrintX=x;PrintY=y;return US_Print(s); }
static bool Draw(const display_settings *s)
{
    ClearMScreen();DrawStripes(10);DrawWindow(20,34,280,144,BKGDCOLOR);
    fontnumber=1;fontcolor=TEXTCOLOR;int w,h;
    if(!VW_MeasurePropString("Display",&w,&h) || !Text((320-w)/2,14,"Display")) return false;
    fontnumber=0;
    static const char *const labels[]={"Fullscreen","Scaling","Window size","Rendering","3D resolution","Aspect ratio","VSync","Filter","Smooth rendering","Reset defaults"};
    for(unsigned i=0;i<10;++i) {
        char value[40];fontcolor=i==selected?HIGHLIGHT:TEXTCOLOR;
        if(i==0) (void)snprintf(value,sizeof(value),"%s",s->pending?"Switching...":s->fullscreen?"On":"Off");
        else if(i==1) (void)snprintf(value,sizeof(value),"%s",s->integer?"Integer":"Fit");
        else if(i==2) (void)snprintf(value,sizeof(value),"%d x %d",s->width,s->height);
        else if(i==3) (void)snprintf(value,sizeof(value),"%s",s->high_resolution?"High Resolution":"Original");
        else if(i==4) { if(s->render_auto) (void)snprintf(value,sizeof(value),"Auto (desktop)");
            else (void)snprintf(value,sizeof(value),"%u x %u",s->render_width,s->render_height); }
        else if(i==5) { static const char *const names[]={"Auto","4:3","16:9","16:10","21:9","32:9"};(void)snprintf(value,sizeof(value),"%s",names[s->aspect<6?s->aspect:0]); }
        else if(i==6) (void)snprintf(value,sizeof(value),"%s",s->vsync?"On":"Off");
        else if(i==7) (void)snprintf(value,sizeof(value),"%s",s->linear?"Linear":"Nearest");
        else if(i==8) (void)snprintf(value,sizeof(value),"%s",s->smooth?"On":"Off");
        else (void)snprintf(value,sizeof(value),"Enter");
        if(!Text(56,39+(int)i*11,labels[i]) || !Text(176,39+(int)i*11,value)) return false;
    }
    fontcolor=TEXTCOLOR;
    if(!Text(26,166,*s->error?"Display change failed; try again.":(s->integer && (s->pixel_width<320 || s->pixel_height<240)?"Small window uses fit scaling.":(s->smooth && !s->high_resolution?"Smooth needs High Resolution.":"F12 saves a screenshot.")))) return false;
    return VWB_DrawPic(24,37+(int)selected*11,C_CURSOR1PIC) && VWB_DrawPic(112,184,C_MOUSELBACKPIC);
}
bool BeginDisplayMenu(void)
{
    if(!Display_Available() || !Get(&previous)) return false;
    IN_ClearKeysDown();return Draw(&previous);
}
int HandleDisplayMenu(void)
{
    if(IN_ConsumePress(sc_Escape)) return -1;
    display_settings s;if(!Display_Available() || !Get(&s)) return -1;
    bool up=IN_ConsumePress(sc_UpArrow),down=IN_ConsumePress(sc_DownArrow);
    bool left=IN_ConsumePress(sc_LeftArrow),right=IN_ConsumePress(sc_RightArrow),enter=IN_ConsumePress(sc_Enter);
    if(up || down) selected=(selected+(up?9U:1U))%10U;
    if((left || right || enter) && !s.pending) {
        if(selected==0) s.fullscreen=!s.fullscreen;
        else if(selected==1) s.integer=!s.integer;
        else if(selected==2) CycleWindowSize(&s,left);
        else if(selected==3) s.high_resolution=!s.high_resolution;
        else if(selected==4) {
            unsigned n=s.render_auto?3U:s.render_width==1920?2U:s.render_width==1280?1U:0U;
            n=left?(n+3)%4:(n+1)%4;
            static const unsigned widths[]={640,1280,1920},heights[]={480,720,1080};
            s.render_auto=n==3;if(n<3) { s.render_width=widths[n];s.render_height=heights[n]; }
        } else if(selected==5) s.aspect=(s.aspect+(left?5U:1U))%6U;
        else if(selected==6) s.vsync=!s.vsync;
        else if(selected==7) s.linear=!s.linear;
        else if(selected==8) s.smooth=!s.smooth;
        else if(enter) { s.smooth=false;s.fullscreen=s.integer=s.high_resolution=s.render_auto=s.vsync=s.linear=false;s.aspect=0;s.scale=3;s.render_width=640;s.render_height=480; }
        (void)Set(&s);if(!Get(&s)) return -1;
    }
    if(up || down || left || right || enter || memcmp(&previous,&s,sizeof(s))) {
        previous=s;return Draw(&s)?0:-3;
    }
    return 0;
}
