/* SPDX-License-Identifier: GPL-2.0-only */
/* WL_MENU.C by John Romero (C) 1992 Id Software, Inc.
   GOODTIMES control-panel slice; blocking HandleMenu is now tic-driven.
   Gameplay-dependent rows retain original disabled rendering and are skipped. */
#include "WL_MENU.h"
#include "WL_SAVE.h"
#include "ID_CA.h"
#include "ID_VH.h"
#include "ID_US_1.h"
#include "ID_IN.h"
#include "ID_SD.h"
#include <ctype.h>
#include <string.h>
#include <stdio.h>
CP_iteminfo MainItems = {76, 55, MENU_COUNT, viewscores, 24};
CP_itemtype MainMenu[MENU_COUNT] = {
    {0,"New Game"}, {0,"Sound"}, {1,"Control"}, {0,"Load Game"}, {0,"Save Game"},
    {0,"Change View"}, {1,"View Scores"}, {1,"Back to Demo"}, {1,"Quit"}
};
static int shape = C_CURSOR1PIC;
static uint64_t gun_next, move_next;
static bool gun_first;
void ClearMScreen(void) { VWB_Bar(0, 0, 320, 200, BORDCOLOR); }
void DrawStripes(int y) { VWB_Bar(0, y, 320, 24, 0); VWB_Hlin(0, 319, y + 22, STRIPE); }
static void DrawOutline(int x, int y, int w, int h, w3r_byte color1, w3r_byte color2)
{
    VWB_Hlin(x,x+w,y,color2); VWB_Vlin(y,y+h,x,color2);
    VWB_Hlin(x,x+w,y+h,color1); VWB_Vlin(y,y+h,x+w,color1);
}
void DrawWindow(int x, int y, int w, int h, w3r_byte color)
{
    VWB_Bar(x,y,w,h,color); DrawOutline(x,y,w,h,BORD2COLOR,DEACTIVE);
}
bool DrawMainMenu(void)
{
    ClearMScreen();
    if (!VWB_DrawPic(112,184,C_MOUSELBACKPIC)) return false;
    DrawStripes(10);
    if (!VWB_DrawPic(84,0,C_OPTIONSPIC)) return false;
    DrawWindow(68,52,178,13*10+6,BKGDCOLOR);
    fontnumber = 1; WindowX = 100;
    for (int i = 0; i < MENU_COUNT; ++i) {
        fontcolor = MainMenu[i].active ? (i == MainItems.curpos ? HIGHLIGHT : TEXTCOLOR) : DEACTIVE;
        PrintX = 100; PrintY = 55 + i * 13;
        if (!US_Print(MainMenu[i].string)) return false;
    }
    return VWB_DrawPic(72,53 + MainItems.curpos * 13,shape);
}
bool SetupControlPanel(uint64_t ticks)
{
    if (!VW_ValidateFont(1)) return false;
    MainMenu[loadgame].active=*Save_Directory()?1:0;
    MainMenu[savegame].active=GameCanSave() && *Save_Directory()?1:0;
    MainMenu[backtodemo].string=GameRunning()?"Back to Game":"Back to Demo";
    if (MainItems.curpos < 0 || MainItems.curpos >= MENU_COUNT || !MainMenu[MainItems.curpos].active) MainItems.curpos = viewscores;
    shape = C_CURSOR1PIC; gun_first = true; gun_next = ticks + 9; move_next = ticks;
    return DrawMainMenu();
}
static void MoveGun(int direction)
{
    do { MainItems.curpos = (MainItems.curpos + direction + MENU_COUNT) % MENU_COUNT; }
    while (!MainMenu[MainItems.curpos].active);
    shape = C_CURSOR1PIC;
    (void)SD_PlaySound(MOVEGUN1SND,0,0);
}
int HandleMenu(uint64_t ticks)
{
    bool redraw = false;
    bool up = IN_ConsumePress(sc_UpArrow), down = IN_ConsumePress(sc_DownArrow);
    if (up || down || ((Keyboard[sc_UpArrow] || Keyboard[sc_DownArrow]) && ticks >= move_next)) {
        int direction = (up || (!down && Keyboard[sc_UpArrow])) ? -1 : 1;
        uint64_t moves = up || down ? 1 : (ticks - move_next) / 20 + 1;
        unsigned available = 0;
        for (int i = 0; i < MENU_COUNT; ++i) if (MainMenu[i].active) ++available;
        /* Repeated cycles return to the same selection; avoid catch-up loops. */
        for (uint64_t n = 0; n < moves % available; ++n) MoveGun(direction);
        move_next = up || down ? ticks + 20 : move_next + moves * 20;
        redraw = true;
    }
    if (!Keyboard[sc_UpArrow] && !Keyboard[sc_DownArrow]) move_next = ticks;
    char ascii;
    while ((ascii=IN_NextASCII()) != 0) {
        int key = toupper((unsigned char)ascii);
        for (int offset = 1; offset <= MENU_COUNT; ++offset) {
            int item = (MainItems.curpos + offset) % MENU_COUNT;
            if (MainMenu[item].active && MainMenu[item].string[0] == key) {
                MainItems.curpos = item; shape = C_CURSOR1PIC; redraw = true; break;
            }
        }
    }
    if (ticks >= gun_next) {
        /* Original first 8-tic blink, then alternate 8/70-tic intervals. */
        if (gun_first) { shape = C_CURSOR2PIC; gun_next += 9; gun_first = false; }
        if (ticks >= gun_next) gun_next += ((ticks-gun_next)/80)*80;
        while (ticks >= gun_next) {
            if (shape == C_CURSOR2PIC) { shape = C_CURSOR1PIC; gun_next += 71; }
            else { shape = C_CURSOR2PIC; gun_next += 9; }
        }
        redraw = true;
    }
    if (redraw && !DrawMainMenu()) return -3;
    bool escape=IN_ConsumePress(sc_Escape);
    bool enter=IN_ConsumePress(sc_Enter), space=IN_ConsumePress(sc_Space);
    if (escape) { (void)SD_PlaySound(ESCPRESSEDSND,0,0);return -1; }
    if (enter || space) { (void)SD_PlaySound(SHOOTSND,0,0);return MainItems.curpos; }
    return -2;
}
bool Message(const char *text)
{
    char line[256];
    int width = 0, height = 0, line_height = 0;
    fontnumber = 1;
    const char *p = text;
    while (*p) {
        const char *end = strchr(p,'\n'); size_t size = end ? (size_t)(end-p) : strlen(p);
        int w;
        if (size >= sizeof(line)) return CA_Fail("Dialog line too long");
        memcpy(line,p,size); line[size]=0;
        if (!VW_MeasurePropString(line,&w,&line_height)) return false;
        if (w > width) width = w;
        height += line_height;
        if (!end) break;
        p = end+1;
    }
    width += 10;
    PrintY = 100 - height/2; PrintX = WindowX = 160-width/2;
    DrawWindow(WindowX-5,PrintY-5,width+10,height+10,TEXTCOLOR);
    DrawOutline(WindowX-5,PrintY-5,width+10,height+10,0,HIGHLIGHT);
    fontcolor = 0;
    return US_Print(text);
}

/* WL_MENU.C DrawNewEpisode/DrawNewGame. Both use the same input queue as
   the control panel; a selection clears edges before entering the next page. */
bool DrawNewSelection(bool episode,unsigned choice)
{
    static const char *episodes[]={"Escape from Wolfenstein","Operation: Eisenfaust","Die, Fuhrer, Die!","A Dark Secret","Trail of the Madman","Confrontation"};
    static const char *skills[]={"Can I play, Daddy?","Don't hurt me.","Bring 'em on!","I am Death incarnate!"};
    ClearMScreen();
    if (!VWB_DrawPic(112,184,C_MOUSELBACKPIC)) return false;
    fontnumber=1;fontcolor=TEXTCOLOR;
    if (episode) {
        DrawWindow(6,19,308,162,BKGDCOLOR);
        PrintX=72;PrintY=2;if (!US_Print("Which episode to play?")) return false;
        for (unsigned i=0;i<6;++i) {
            if (!VWB_DrawPic(42,23+(int)i*26,C_EPISODE1PIC+(int)i)) return false;
            fontcolor=i==choice ? HIGHLIGHT : TEXTCOLOR;
            char label[80];(void)snprintf(label,sizeof(label),"Episode %u\n%s",i+1,episodes[i]);
            PrintX=98;PrintY=23+(int)i*26;if (!US_Print(label)) return false;
        }
        return VWB_DrawPic(10,24+(int)choice*26,C_CURSOR1PIC);
    }
    PrintX=70;PrintY=68;if (!US_Print("How tough are you?")) return false;
    DrawWindow(45,90,225,67,BKGDCOLOR);
    for (unsigned i=0;i<4;++i) {
        fontcolor=i==choice ? HIGHLIGHT : TEXTCOLOR;
        PrintX=74;PrintY=100+(int)i*13;if (!US_Print(skills[i])) return false;
    }
    return VWB_DrawPic(50,98+(int)choice*13,C_CURSOR1PIC) &&
        VWB_DrawPic(235,107,C_BABYMODEPIC+(int)choice);
}
int HandleNewSelection(uint64_t ticks,bool episode,unsigned *choice)
{
    unsigned count=episode ? 6U : 4U;
    bool up=IN_ConsumePress(sc_UpArrow),down=IN_ConsumePress(sc_DownArrow);
    if (up || down || ((Keyboard[sc_UpArrow] || Keyboard[sc_DownArrow]) && ticks>=move_next)) {
        *choice=(*choice+((up || (!down && Keyboard[sc_UpArrow])) ? count-1 : 1))%count;
        move_next=ticks+20;
        (void)SD_PlaySound(MOVEGUN1SND,0,0);
        if (!DrawNewSelection(episode,*choice)) return -3;
    }
    bool escape=IN_ConsumePress(sc_Escape),enter=IN_ConsumePress(sc_Enter),space=IN_ConsumePress(sc_Space);
    if (escape) { (void)SD_PlaySound(ESCPRESSEDSND,0,0);return -1; }
    if (enter || space) { (void)SD_PlaySound(SHOOTSND,0,0);return 1; }
    return 0;
}

/* Minimal functional settings within the existing control-panel renderer. */
static unsigned sound_choice;
bool DrawSoundSettings(void)
{
    unsigned music,effects;bool mon,eon;SD_GetSettings(&music,&effects,&mon,&eon);
    ClearMScreen();DrawWindow(36,32,248,132,BKGDCOLOR);
    fontnumber=1;fontcolor=TEXTCOLOR;PrintX=112;PrintY=12;
    if(!US_Print("Sound")) return false;
    char text[4][64];
    (void)snprintf(text[0],sizeof(text[0]),"Music: %s",mon?"On":"Off");
    (void)snprintf(text[1],sizeof(text[1]),"Effects: %s",eon?"On":"Off");
    (void)snprintf(text[2],sizeof(text[2]),"Music volume: %u%%",music);
    (void)snprintf(text[3],sizeof(text[3]),"Effects volume: %u%%",effects);
    for(unsigned i=0;i<4;++i) {
        fontcolor=i==sound_choice?HIGHLIGHT:TEXTCOLOR;PrintX=62;PrintY=48+(int)i*24;
        if(!US_Print(text[i])) return false;
    }
    fontcolor=TEXTCOLOR;PrintX=40;PrintY=176;
    return US_Print("Arrows adjust; Esc returns");
}
int HandleSoundSettings(void)
{
    bool up=IN_ConsumePress(sc_UpArrow),down=IN_ConsumePress(sc_DownArrow);
    bool left=IN_ConsumePress(sc_LeftArrow),right=IN_ConsumePress(sc_RightArrow);
    bool enter=IN_ConsumePress(sc_Enter),space=IN_ConsumePress(sc_Space);
    if(IN_ConsumePress(sc_Escape)) { (void)SD_PlaySound(ESCPRESSEDSND,0,0);return -1; }
    if(up || down) sound_choice=(sound_choice+(up?3U:1U))%4;
    if(left || right || enter || space) {
        unsigned m,e;bool mon,eon;SD_GetSettings(&m,&e,&mon,&eon);
        if(sound_choice==0) mon=!mon;
        else if(sound_choice==1) eon=!eon;
        else {
            unsigned *v=sound_choice==2?&m:&e;
            if(left) *v=*v>=5?*v-5:0;
            else *v=*v<=95?*v+5:100;
        }
        SD_SetVolumes(m,e);SD_SetEnabled(mon,eon);
    }
    if((up || down || left || right || enter || space) && !DrawSoundSettings()) return -3;
    return 0;
}
