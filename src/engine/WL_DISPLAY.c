/* SPDX-License-Identifier: GPL-2.0-only */
#include "WL_DISPLAY.h"
#include "WL_MENU.h"
#include "ID_IN.h"
#include "ID_VH.h"
#include "ID_US_1.h"
#include <stdio.h>
#include <string.h>
static bool (*Get)(display_settings *),(*Set)(const display_settings *);
static unsigned selected;
static display_settings previous;
void Display_Connect(bool (*get)(display_settings *),bool (*set)(const display_settings *)) { Get=get;Set=set; }
bool Display_Available(void) { return Get && Set; }
static bool Text(int x,int y,const char *s) { PrintX=x;PrintY=y;return US_Print(s); }
static bool Draw(const display_settings *s)
{
    ClearMScreen();DrawStripes(10);DrawWindow(20,38,280,140,BKGDCOLOR);
    fontnumber=1;fontcolor=TEXTCOLOR;if(!Text(87,14,"Display")) return false;
    fontnumber=0;
    static const char *const labels[]={"Fullscreen","Scaling","Window size","Rendering","3D resolution","Aspect ratio","VSync","Filter","Reset defaults"};
    for(unsigned i=0;i<9;++i) {
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
        else (void)snprintf(value,sizeof(value),"Enter");
        if(!Text(30,43+(int)i*13,labels[i]) || !Text(176,43+(int)i*13,value)) return false;
    }
    fontcolor=TEXTCOLOR;
    if(!Text(26,166,*s->error?"Display change failed; try again.":(s->integer && (s->pixel_width<320 || s->pixel_height<240)?"Small window uses fit scaling.":"F12 saves a screenshot."))) return false;
    return Text(20,182,"Arrows change; Enter selects.");
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
    if(up || down) selected=(selected+(up?8U:1U))%9U;
    if((left || right || enter) && !s.pending) {
        if(selected==0) s.fullscreen=!s.fullscreen;
        else if(selected==1) s.integer=!s.integer;
        else if(selected==2) { unsigned n=s.scale?s.scale:3U;s.scale=left?(n>1?n-1:1):(n<5?n+1:5); }
        else if(selected==3) s.high_resolution=!s.high_resolution;
        else if(selected==4) {
            unsigned n=s.render_auto?3U:s.render_width==1920?2U:s.render_width==1280?1U:0U;
            n=left?(n+3)%4:(n+1)%4;
            static const unsigned widths[]={640,1280,1920},heights[]={480,720,1080};
            s.render_auto=n==3;if(n<3) { s.render_width=widths[n];s.render_height=heights[n]; }
        } else if(selected==5) s.aspect=(s.aspect+(left?5U:1U))%6U;
        else if(selected==6) s.vsync=!s.vsync;
        else if(selected==7) s.linear=!s.linear;
        else if(enter) { s.fullscreen=s.integer=s.high_resolution=s.render_auto=s.vsync=s.linear=false;s.aspect=0;s.scale=3;s.render_width=640;s.render_height=480; }
        (void)Set(&s);if(!Get(&s)) return -1;
    }
    if(up || down || left || right || enter || memcmp(&previous,&s,sizeof(s))) {
        previous=s;return Draw(&s)?0:-3;
    }
    return 0;
}
