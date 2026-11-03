/* SPDX-License-Identifier: GPL-2.0-only */
#include "WL_CONTROLS.h"
#include "WL_MENU.h"
#include "WL_DISPLAY.h"
#include "ID_IN.h"
#include "ID_VH.h"
#include "ID_CA.h"
#include "ID_US_1.h"
#include "ID_SD.h"
#include <stdio.h>
static unsigned row,binding,alternative,control_row;
static enum { CONTROL_ROOT,CONTROL_OPTIONS,CONTROL_SENSITIVITY } page;
static bool bindings_page,waiting,conflict;
static const char *KeyName(unsigned key,char text[24])
{
    if(!key) return "-";
    if(key>=4 && key<=29) { text[0]=(char)('A'+key-4);text[1]=0;return text; }
    if(key>=30 && key<=38) { text[0]=(char)('1'+key-30);text[1]=0;return text; }
    if(key>=58 && key<=69) { (void)snprintf(text,24,"F%u",key-57);return text; }
    if(key>=89 && key<=97) { (void)snprintf(text,24,"KP %u",key-88);return text; }
    switch(key) {
    case 39:return "0";case 40:return "Enter";case 41:return "Esc";case 42:return "Backspace";
    case 43:return "Tab";case 44:return "Space";case 79:return "Right";case 80:return "Left";
    case 57:return "Caps Lock";case 73:return "Insert";case 74:return "Home";case 75:return "Page Up";
    case 76:return "Delete";case 77:return "End";case 78:return "Page Down";case 98:return "KP 0";
    case 81:return "Down";case 82:return "Up";case 224:return "L Ctrl";case 225:return "L Shift";
    case 226:return "L Alt";case 228:return "R Ctrl";case 229:return "R Shift";case 230:return "R Alt";
    default:(void)snprintf(text,24,"Key %u",key);return text;
    }
}
static bool Text(int x,int y,const char *text) { PrintX=x;PrintY=y;return US_Print(text); }
static bool DrawOptions(void)
{
    ClearMScreen();DrawStripes(10);DrawWindow(20,50,280,124,BKGDCOLOR);
    fontnumber=1;fontcolor=TEXTCOLOR;
    if(!VWB_DrawPic(80,0,C_CUSTOMIZEPIC)) return false;
    fontnumber=0;
    if(bindings_page) {
        if(!Text(30,53,"Action") || !Text(171,53,"Key 1") || !Text(234,53,"Key 2")) return false;
        unsigned first=(binding/5)*5;
        for(unsigned i=first;i<first+5 && i<CT_ACTIONS;++i) {
            int y=65+(int)(i-first)*17;fontcolor=i==binding?HIGHLIGHT:TEXTCOLOR;
            if(!Text(56,y,CT_ActionName((ct_action)i))) return false;
            for(unsigned slot=0;slot<2;++slot) {
                char key[24];fontcolor=i==binding && slot==alternative?HIGHLIGHT:TEXTCOLOR;
                if(!Text(slot?234:171,y,KeyName(CT_Binding((ct_action)i,slot),key))) return false;
            }
        }
        if(!VWB_DrawPic(24,63+(int)(binding-first)*17,MenuCursorPicture())) return false;
        fontcolor=HIGHLIGHT;
        if(!Text(30,153,waiting?(conflict?"Key reserved or in use.":"Press a key; Esc cancels."):"Enter binds; Delete clears.")) return false;
        fontcolor=TEXTCOLOR;return VWB_DrawPic(112,184,C_MOUSELBACKPIC);
    }
    static const char *const labels[]={"Preset","Mouse","Sensitivity","Invert mouse X","Reset defaults","Key bindings","Display settings"};
    for(unsigned i=0;i<(Display_Available()?7U:6U);++i) {
        fontcolor=i==row?HIGHLIGHT:TEXTCOLOR;int y=52+(int)i*16;char value[32];
        if(!Text(56,y,labels[i])) return false;
        if(i==0) (void)snprintf(value,sizeof(value),"%s",CT_Preset()==CT_CLASSIC?"Classic":"Modern");
        else if(i==1) (void)snprintf(value,sizeof(value),"%s",CT_MouseEnabled()?"On":"Off");
        else if(i==2) (void)snprintf(value,sizeof(value),"%u / 40",CT_Sensitivity());
        else if(i==3) (void)snprintf(value,sizeof(value),"%s",CT_Invert()?"On":"Off");
        else (void)snprintf(value,sizeof(value),"Enter");
        if(!Text(207,y,value)) return false;
    }
    if(!VWB_DrawPic(24,50+(int)row*16,MenuCursorPicture())) return false;
    fontcolor=TEXTCOLOR;return VWB_DrawPic(112,184,C_MOUSELBACKPIC);
}
static bool ControlActive(unsigned item)
{ return item==0 || item==5 || (item==4 && CT_MouseEnabled()) || (item==6 && Display_Available()); }
static bool Draw(void)
{
    if(page==CONTROL_OPTIONS) return DrawOptions();
    ClearMScreen();fontnumber=1;fontcolor=TEXTCOLOR;
    if(!VWB_DrawPic(112,184,C_MOUSELBACKPIC)) return false;
    if(page==CONTROL_SENSITIVITY) {
        DrawWindow(10,80,300,30,BKGDCOLOR);
        int w,h;const char *title="Adjust Mouse Sensitivity";
        if(!VW_MeasurePropString(title,&w,&h) || !Text((320-w)/2,82,title) ||
           !Text(14,95,"Slow") || !Text(269,95,"Fast")) return false;
        VWB_Bar(60,97,200,10,TEXTCOLOR);
        unsigned offset=(CT_Sensitivity()-1)*180/39;
        DrawWindow(60+(int)offset,97,20,10,HIGHLIGHT);
        return true;
    }
    DrawStripes(10);
    if(!VWB_DrawPic(80,0,C_CONTROLPIC)) return false;
    DrawWindow(16,62,284,102,BKGDCOLOR);
    static const char *const labels[]={"Mouse Enabled","Joystick Enabled","Use joystick port 2",
        "Gravis GamePad Enabled","Mouse Sensitivity","Customize controls","Display settings"};
    for(unsigned i=0;i<7;++i) {
        fontcolor=ControlActive(i)?(i==control_row?HIGHLIGHT:TEXTCOLOR):DEACTIVE;
        if(!Text(80,70+(int)i*13,labels[i])) return false;
        if(i<4 && !VWB_DrawPic(56,73+(int)i*13,i==0 && CT_MouseEnabled()?C_SELECTEDPIC:C_NOTSELECTEDPIC)) return false;
    }
    return VWB_DrawPic(24,68+(int)control_row*13,MenuCursorPicture());
}
bool DrawControlsMenu(void) { return Draw(); }
bool ResumeControlsMenu(void) { IN_ClearKeysDown();return Draw(); }
bool BeginControlsMenu(void)
{ MenuCursorMoved();control_row=row=0;page=CONTROL_ROOT;bindings_page=waiting=conflict=false;return ResumeControlsMenu(); }
static int HandleOptions(void)
{
    if(waiting) {
        if(IN_ConsumePress(sc_Escape)) { waiting=false;IN_ClearKeysDown();return Draw()?0:-3; }
        unsigned key=CT_NextKey();
        if(key) {
            if(CT_Bind((ct_action)binding,alternative,key)) waiting=false;
            else conflict=true;
            IN_ClearKeysDown();return Draw()?0:-3;
        }
        return 0;
    }
    if(IN_ConsumePress(sc_Escape)) {
        (void)SD_PlaySound(ESCPRESSEDSND,0,0);
        if(bindings_page) { bindings_page=false;return Draw()?0:-3; }
        return -1;
    }
    int direction=MenuNavigation();bool up=direction<0,down=direction>0;
    bool left=IN_ConsumePress(sc_LeftArrow),right=IN_ConsumePress(sc_RightArrow);
    bool enter=IN_ConsumePress(sc_Enter);
    if(up || down) { MenuCursorMoved();(void)SD_PlaySound(MOVEGUN1SND,0,0); }
    if(bindings_page) {
        if(up || down) binding=(binding+(up?CT_ACTIONS-1U:1U))%CT_ACTIONS;
        if(left || right) alternative^=1U;
        if(IN_ConsumePress(0x53)) { (void)CT_Bind((ct_action)binding,alternative,0);return Draw()?0:-3; }
        if(enter) { waiting=true;conflict=false;IN_ClearKeysDown(); }
        return !(up || down || left || right || enter) || Draw()?0:-3;
    }
    if(up || down) row=(row+(up?(Display_Available()?6U:5U):1U))%(Display_Available()?7U:6U);
    if(left || right || enter) {
        if(row==0) CT_SelectPreset(CT_Preset()==CT_CLASSIC?CT_MODERN:CT_CLASSIC);
        else if(row==1) CT_SetMouse(!CT_MouseEnabled());
        else if(row==2) {
            unsigned sensitivity=CT_Sensitivity();
            if(left && sensitivity>1) --sensitivity;
            if((right || enter) && sensitivity<40) ++sensitivity;
            CT_SetSensitivity(sensitivity);
        } else if(row==3) CT_SetInvert(!CT_Invert());
        else if(row==4 && enter) CT_Reset(CT_Preset());
        else if(row==6 && enter) return 2;
        else if(row==5 && enter) { bindings_page=true;binding=alternative=0; }
        if(enter) (void)SD_PlaySound(SHOOTSND,0,0);
        IN_ClearKeysDown();
    }
    return !(up || down || left || right || enter) || Draw()?0:-3;
}

int HandleControlsMenu(void)
{
    if(page==CONTROL_OPTIONS) {
        int action=HandleOptions();
        if(action==-1) { page=CONTROL_ROOT;IN_ClearKeysDown();return Draw()?0:-3; }
        return action;
    }
    if(IN_ConsumePress(sc_Escape)) {
        (void)SD_PlaySound(ESCPRESSEDSND,0,0);
        if(page==CONTROL_ROOT) return -1;
        page=CONTROL_ROOT;IN_ClearKeysDown();return Draw()?0:-3;
    }
    int direction=MenuNavigation();bool up=direction<0,down=direction>0;
    bool left=IN_ConsumePress(sc_LeftArrow),right=IN_ConsumePress(sc_RightArrow);
    bool enter=IN_ConsumePress(sc_Enter)|IN_ConsumePress(sc_Space);
    if(enter) (void)SD_PlaySound(SHOOTSND,0,0);
    if(up || down) { MenuCursorMoved();(void)SD_PlaySound(MOVEGUN1SND,0,0); }
    if(page==CONTROL_SENSITIVITY) {
        unsigned value=CT_Sensitivity();
        if((left || down) && value>1) --value;
        if((right || up) && value<40) ++value;
        CT_SetSensitivity(value);
        if(enter) page=CONTROL_ROOT;
    } else {
        if(up || down) do { control_row=(control_row+(up?6U:1U))%7; } while(!ControlActive(control_row));
        if(left || right || enter) {
            if(control_row==0) CT_SetMouse(!CT_MouseEnabled());
            else if(control_row==4 && enter) page=CONTROL_SENSITIVITY;
            else if(control_row==5 && enter) { page=CONTROL_OPTIONS;row=0; }
            else if(control_row==6 && enter && Display_Available()) return 2;
        }
    }
    return !(up || down || left || right || enter) || Draw()?0:-3;
}
