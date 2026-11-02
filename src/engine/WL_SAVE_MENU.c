/* SPDX-License-Identifier: GPL-2.0-only */
/* WL_MENU.C DrawLoadSaveScreen/CP_SaveGame/CP_LoadGame, resumed through the
   existing menu input queue. Six slots as requested; no separate UI toolkit. */
#include "WL_MENU.h"
#include "WL_SAVE.h"
#include "ID_VH.h"
#include "ID_US_1.h"
#include "ID_IN.h"
#include "ID_CA.h"
#include "ID_SD.h"
#include <stdio.h>
#include <string.h>
static save_info entries[SAVE_SLOTS];
static unsigned selected;
static bool saving;
static enum { PICK_SLOT,OVERWRITE,EDIT_NAME,SLOT_ERROR } phase;
static char name[SAVE_DESCRIPTION];
static bool DrawSlots(void)
{
    ClearMScreen();DrawStripes(10);
    if(!VWB_DrawPic(60,0,saving?C_SAVEGAMEPIC:C_LOADGAMEPIC) || !VWB_DrawPic(112,184,C_MOUSELBACKPIC)) return false;
    DrawWindow(75,50,175,6*13+10,BKGDCOLOR);
    fontnumber=0;
    for(unsigned i=0;i<SAVE_SLOTS;++i) {
        const save_info *e=&entries[i];char line[SAVE_DESCRIPTION];
        const char *label=e->status==SAVE_EMPTY?"- empty -":e->status==SAVE_UNSUPPORTED?"Unsupported version":e->status==SAVE_CORRUPT?"Corrupt save":e->status==SAVE_UNAVAILABLE?"Storage unavailable":e->description;
        (void)snprintf(line,sizeof(line),"%s",phase==EDIT_NAME && i==selected?name:label);
        int width,height;
        while(*line) {
            if(!VW_MeasurePropString(line,&width,&height)) return false;
            if(width<=121) break;
            line[strlen(line)-1]=0;
        }
        fontcolor=i==selected?HIGHLIGHT:TEXTCOLOR;PrintX=111;PrintY=56+(int)i*13;
        if(!US_Print(line)) return false;
    }
    if(!VWB_DrawPic(85,53+(int)selected*13,C_CURSOR1PIC)) return false;
    fontcolor=TEXTCOLOR;char details[80];
    if(entries[selected].status==SAVE_VALID)
        (void)snprintf(details,sizeof(details),"Slot %u  E%u / Floor %u / Skill %u",selected+1,entries[selected].episode+1,entries[selected].level+1,entries[selected].difficulty+1);
    else (void)snprintf(details,sizeof(details),"Slot %u",selected+1);
    PrintX=75;PrintY=147;if(!US_Print(details)) return false;
    PrintX=75;PrintY=163;
    return US_Print(phase==EDIT_NAME?"Enter saves; Esc cancels":"Enter selects; Esc returns");
}
bool BeginSaveMenu(bool save)
{
    saving=save;phase=PICK_SLOT;
    for(unsigned i=0;i<SAVE_SLOTS;++i) Save_ReadInfo(i,&entries[i]);
    return DrawSlots();
}
static bool BeginName(void)
{
    phase=EDIT_NAME;
    (void)snprintf(name,sizeof(name),"%s",entries[selected].status==SAVE_VALID?entries[selected].description:"");
    IN_ClearKeysDown();return DrawSlots();
}
static bool Error(const char *text)
{
    phase=SLOT_ERROR;IN_ClearKeysDown();fontnumber=1;
    /* Keep error dialogs inside the original 320-pixel layout. */
    char message[384];size_t at=0;unsigned column=0;
    for(const char *p=text;*p && at<sizeof(message)-40;++p) {
        if(*p==' ' && column>=29) { message[at++]='\n';column=0; }
        else { message[at++]=*p;++column; }
    }
    (void)strcpy(message+at,"\nPress a key to return.");return Message(message);
}
int HandleSaveMenu(uint64_t ticks)
{
    if(phase==SLOT_ERROR) {
        if(IN_AnyPressed()) { phase=PICK_SLOT;IN_ClearKeysDown();if(!DrawSlots()) return -3; }
        return 0;
    }
    if(phase==OVERWRITE) {
        if(IN_ConsumePress(sc_Y)) { if(!BeginName()) return -3; }
        else if(IN_ConsumePress(sc_N) || IN_ConsumePress(sc_Escape)) { phase=PICK_SLOT;IN_ClearKeysDown();if(!DrawSlots()) return -3; }
        return 0;
    }
    if(phase==EDIT_NAME) {
        if(IN_ConsumePress(sc_Escape)) { phase=PICK_SLOT;IN_ClearKeysDown();return DrawSlots()?0:-3; }
        bool redraw=false;
        if(IN_ConsumePress(0x0e) && *name) { name[strlen(name)-1]=0;redraw=true; }
        char ch;
        while((ch=IN_NextASCII())!=0) {
            size_t n=strlen(name);
            if(n>=SAVE_DESCRIPTION-1) continue;
            name[n]=ch;name[n+1]=0;int width,height;fontnumber=0;
            if(!VW_MeasurePropString(name,&width,&height)) return -3;
            if(width>121) name[n]=0;else redraw=true;
        }
        if(IN_ConsumePress(sc_Enter)) {
            if(!*name) (void)snprintf(name,sizeof(name),"E%d Floor %d",gamestate.episode+1,gamestate.mapon+1);
            if(!Save_Write(selected,name,entries[selected].status!=SAVE_EMPTY)) return Error(CA_Error())?0:-3;
            (void)SD_PlaySound(SHOOTSND,0,0);return 1;
        }
        return !redraw || DrawSlots()?0:-3;
    }
    if(IN_ConsumePress(sc_Escape)) { (void)SD_PlaySound(ESCPRESSEDSND,0,0);return -1; }
    bool up=IN_ConsumePress(sc_UpArrow),down=IN_ConsumePress(sc_DownArrow);
    if(up || down) { selected=(selected+(up?SAVE_SLOTS-1U:1U))%SAVE_SLOTS;(void)SD_PlaySound(MOVEGUN1SND,0,0);if(!DrawSlots()) return -3; }
    if(IN_ConsumePress(sc_Enter) || IN_ConsumePress(sc_Space)) {
        if(saving) {
            if(!GameCanSave()) return Error("No active game can be saved.")?0:-3;
            if(entries[selected].status==SAVE_EMPTY) return BeginName()?0:-3;
            phase=OVERWRITE;IN_ClearKeysDown();fontnumber=1;
            return Message(entries[selected].status==SAVE_VALID?"Overwrite this save?\nY / N":"Replace this invalid save?\nY / N")?0:-3;
        }
        if(!Save_Load(selected,ticks)) return Error(CA_Error())?0:-3;
        return 1;
    }
    return 0;
}
