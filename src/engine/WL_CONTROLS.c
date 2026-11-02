/* SPDX-License-Identifier: GPL-2.0-only */
#include "WL_CONTROLS.h"
#include <float.h>
#include <string.h>
static ct_preset preset;
static bool gameplay;
void CT_SetGameplay(bool active) { gameplay=active; }
bool CT_Gameplay(void) { return gameplay; }
static unsigned bindings[CT_ACTIONS][2]={
    {82,96},{81,90},{80,92},{79,94},{0,0},{0,0},{225,229},{226,230},
    {224,228},{44,0},{30,0},{31,0},{32,0},{33,0},{41,0}
};
static bool keys[CT_KEYS],pressed[CT_KEYS],buttons[4],clicks[4],mouse,invert;
static unsigned sensitivity=10,nextkey;
static double motion;
void CT_Clear(void) { memset(keys,0,sizeof(keys));memset(buttons,0,sizeof(buttons));motion=0;CT_EndTic(); }
void CT_EndTic(void) { memset(pressed,0,sizeof(pressed));memset(clicks,0,sizeof(clicks));nextkey=0; }
void CT_Reset(ct_preset value)
{
    static const unsigned classic[CT_ACTIONS][2]={
        {82,96},{81,90},{80,92},{79,94},{0,0},{0,0},{225,229},{226,230},
        {224,228},{44,0},{30,0},{31,0},{32,0},{33,0},{41,0}
    };
    if(value!=CT_CLASSIC && value!=CT_MODERN) return;
    memcpy(bindings,classic,sizeof(bindings));preset=value;mouse=value==CT_MODERN;invert=false;sensitivity=10;
    if(value==CT_MODERN) {
        bindings[CT_FORWARD][0]=26;bindings[CT_FORWARD][1]=82;
        bindings[CT_BACK][0]=22;bindings[CT_BACK][1]=81;
        bindings[CT_STRAFE_LEFT][0]=4;bindings[CT_STRAFE_RIGHT][0]=7;
    }
    CT_Clear();
}
ct_preset CT_Preset(void) { return preset; }
void CT_Key(unsigned key,bool down)
{
    if(!key || key>=CT_KEYS) return;
    if(down && !keys[key]) { pressed[key]=true;nextkey=key; }
    keys[key]=down;
}
void CT_LegacyKey(unsigned scan,bool down)
{
    /* Legacy engine/test callers use original set-1 keys. SDL feeds the USB
       keys directly and updates menu state through IN_MenuKeyEvent instead. */
    static const unsigned letters[26]={0x1e,0x30,0x2e,0x20,0x12,0x21,0x22,0x23,0x17,0x24,0x25,0x26,0x32,0x31,0x18,0x19,0x10,0x13,0x1f,0x14,0x16,0x2f,0x11,0x2d,0x15,0x2c};
    for(unsigned i=0;i<26;++i) if(scan==letters[i]) { CT_Key(i+4,down);return; }
    if(scan>=2 && scan<=10) { CT_Key(scan+28,down);return; }
    unsigned key=0;
    switch(scan) {
    case 11:key=39;break;case 1:key=41;break;case 0x1c:key=40;break;case 0x39:key=44;break;
    case 0x48:key=82;break;case 0x50:key=81;break;case 0x4b:key=80;break;case 0x4d:key=79;break;
    case 0x2a:key=225;break;case 0x36:key=229;break;case 0x1d:key=224;break;case 0x38:key=226;break;
    default:break;
    }
    CT_Key(key,down);
}
void CT_MouseButton(unsigned button,bool down)
{
    if(!mouse || button>=4 || !button) return;
    if(down && !buttons[button]) clicks[button]=true;
    buttons[button]=down;
}
void CT_MouseMotion(double x)
{
    if(!mouse || !(x>=-DBL_MAX && x<=DBL_MAX)) return;
    if(x>4096) x=4096;
    if(x< -4096) x=-4096;
    motion+=x*(double)sensitivity*(invert?-1.0:1.0);
    if(motion>7200) motion=7200;
    if(motion< -7200) motion=-7200;
}
int CT_MouseTurn(void) { int value=(int)motion;motion-=value;return value; }
bool CT_Down(ct_action action)
{
    if((unsigned)action>=CT_ACTIONS) return false;
    for(unsigned slot=0;slot<2;++slot) { unsigned key=bindings[action][slot];if(key && (keys[key] || pressed[key])) return true; }
    unsigned button=action==CT_FIRE?1U:action==CT_USE?3U:0U;
    return mouse && button && (buttons[button] || clicks[button]);
}
bool CT_Pressed(ct_action action)
{
    if((unsigned)action>=CT_ACTIONS) return false;
    bool result=false;
    for(unsigned slot=0;slot<2;++slot) { unsigned key=bindings[action][slot];if(key && pressed[key]) { result=true;pressed[key]=false; } }
    unsigned button=action==CT_USE?3U:action==CT_FIRE?1U:0U;
    if(mouse && button && clicks[button]) { result=true;clicks[button]=false; }
    return result;
}
unsigned CT_Binding(ct_action action,unsigned slot) { return (unsigned)action<CT_ACTIONS && slot<2?bindings[action][slot]:0; }
bool CT_Bind(ct_action action,unsigned slot,unsigned key)
{
    if((unsigned)action>=CT_ACTIONS || slot>=2 || key>=CT_KEYS) return false;
    /* Escape always remains a safe global menu/cancel key. */
    if(key==41 && action!=CT_MENU) return false;
    if(key) for(unsigned a=0;a<CT_ACTIONS;++a) for(unsigned s=0;s<2;++s)
        if((a!=(unsigned)action || s!=slot) && bindings[a][s]==key) return false;
    bindings[action][slot]=key;CT_Clear();return true;
}
const char *CT_ActionName(ct_action action)
{
    static const char *const names[]={"Forward","Backward","Turn left","Turn right","Strafe left","Strafe right","Run","Alt strafe","Fire","Use / open","Knife","Pistol","Machine gun","Chaingun","Menu"};
    return (unsigned)action<CT_ACTIONS?names[action]:"Unknown";
}
bool CT_MouseEnabled(void) { return mouse; }
void CT_SetMouse(bool value) { mouse=value;CT_Clear(); }
unsigned CT_Sensitivity(void) { return sensitivity; }
void CT_SetSensitivity(unsigned value) { if(value>=1 && value<=40) { sensitivity=value;motion=0; } }
bool CT_Invert(void) { return invert; }
void CT_SetInvert(bool value) { invert=value;motion=0; }
unsigned CT_NextKey(void) { unsigned key=nextkey;nextkey=0;return key; }
