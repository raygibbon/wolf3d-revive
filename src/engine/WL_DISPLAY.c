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
    ClearMScreen();DrawStripes(10);DrawWindow(20,45,280,126,BKGDCOLOR);
    fontnumber=1;fontcolor=TEXTCOLOR;if(!Text(87,14,"Display")) return false;
    fontnumber=0;
    static const char *const labels[]={"Fullscreen","Integer scaling","Window size","Reset defaults"};
    for(unsigned i=0;i<4;++i) {
        char value[40];fontcolor=i==selected?HIGHLIGHT:TEXTCOLOR;
        if(i==0) (void)snprintf(value,sizeof(value),"%s",s->pending?"Switching...":s->fullscreen?"On":"Off");
        else if(i==1) (void)snprintf(value,sizeof(value),"%s",s->integer?"On":"Off");
        else if(i==2) (void)snprintf(value,sizeof(value),"%d x %d",s->width,s->height);
        else (void)snprintf(value,sizeof(value),"Enter");
        if(!Text(30,56+(int)i*22,labels[i]) || !Text(188,56+(int)i*22,value)) return false;
    }
    fontcolor=TEXTCOLOR;
    if(!Text(30,150,*s->error?"Display change failed; try again.":(s->integer && (s->pixel_width<320 || s->pixel_height<240)?"Small window uses fit scaling.":"4:3 image; black borders as needed."))) return false;
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
    if(up || down) selected=(selected+(up?3U:1U))%4U;
    if((left || right || enter) && !s.pending) {
        if(selected==0) s.fullscreen=!s.fullscreen;
        else if(selected==1) s.integer=!s.integer;
        else if(selected==2) { unsigned n=s.scale?s.scale:3U;s.scale=left?(n>1?n-1:1):(n<5?n+1:5); }
        else if(enter) { s.fullscreen=s.integer=false;s.scale=3; }
        (void)Set(&s);if(!Get(&s)) return -1;
    }
    if(up || down || left || right || enter || memcmp(&previous,&s,sizeof(s))) {
        previous=s;return Draw(&s)?0:-3;
    }
    return 0;
}
