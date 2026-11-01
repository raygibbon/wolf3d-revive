/* SPDX-License-Identifier: GPL-2.0-only */
/* WL_INTER.C DrawHighScores: original default table from ID_US_1.C.
   No save/config import or score-entry gameplay has been ported. */
#include "WL_MENU.h"
#include "ID_VH.h"
#include "ID_CA.h"
#include "ID_US_1.h"
bool DrawHighScores(void)
{
    static const char *names[] = {"id software-'92", "Adrian Carmack", "John Carmack", "Kevin Cloud", "Tom Hall", "John Romero", "Jay Wilbur"};
    const char level[] = {(char)130,0};
    const char score[] = {(char)130,(char)129,(char)129,(char)129,(char)129,0};
    ClearMScreen(); DrawStripes(10);
    if (!VWB_DrawPic(48,0,HIGHSCORESPIC) || !VWB_DrawPic(32,68,C_NAMEPIC) ||
        !VWB_DrawPic(160,68,C_LEVELPIC) || !VWB_DrawPic(224,68,C_SCOREPIC)) return false;
    fontnumber = 0; fontcolor = 15;
    for (int i = 0; i < 7; ++i) {
        int width, height;
        PrintX = 32; PrintY = 76 + 16*i;
        if (!US_Print(names[i]) || !VW_MeasurePropString(level,&width,&height)) return false;
        PrintX = 22*8-width-6;
        if (!US_Print("E1/L") || !US_Print(level) || !VW_MeasurePropString(score,&width,&height)) return false;
        PrintX = 34*8-8-width;
        if (!US_Print(score)) return false;
    }
    return true;
}

/* WL_INTER.C LevelCompleted: original par times, integer ratios and bonuses.
   Results are shown immediately (the original acknowledgement skip path).
   Sound/count-up/BJ breathing animation are not yet ported. */
#include "WL_DEF.h"
#include <stdio.h>
#include <ctype.h>
#include <string.h>
levelratio_t LevelRatios[8];
levelratio_t LevelStats(void)
{
    static const unsigned par[6][8]={
        {90,120,120,210,180,180,150,150},
        {90,210,180,120,240,360,60,180},
        {90,90,150,150,210,150,120,360},
        {120,120,90,60,270,210,120,270},
        {150,90,150,150,240,180,270,210},
        {390,240,270,360,300,330,330,510}
    };
    levelratio_t r={0};
    r.seconds=gamestate.TimeCount/70;
    if (r.seconds>99*60) r.seconds=99*60;
    if (gamestate.mapon>=8) { r.bonus=15000;return r; }
    r.par=par[gamestate.episode][gamestate.mapon];
    if (gamestate.killtotal) r.kill=(unsigned)(gamestate.killcount*100/gamestate.killtotal);
    if (gamestate.secrettotal) r.secret=(unsigned)(gamestate.secretcount*100/gamestate.secrettotal);
    if (gamestate.treasuretotal) r.treasure=(unsigned)(gamestate.treasurecount*100/gamestate.treasuretotal);
    r.bonus=(int)(r.seconds<r.par ? (r.par-r.seconds)*500 : 0)+
        10000*((r.kill==100)+(r.secret==100)+(r.treasure==100));
    return r;
}
static bool Write(int x,int y,const char *text)
{
    int nx=x*8,ny=y*8;
    for (const char *p=text;*p;++p) {
        int ch=toupper((unsigned char)*p),pic=-1,advance=16;
        if (ch=='\n') { nx=x*8;ny+=16;continue; }
        if (ch>='0' && ch<='9') pic=L_NUM0PIC+ch-'0';
        else if (ch>='A' && ch<='Z') pic=L_APIC+ch-'A';
        else if (ch==':') { pic=L_COLONPIC;advance=8; }
        else if (ch=='!') { pic=L_EXPOINTPIC;advance=8; }
        else if (ch=='%') pic=L_PERCENTPIC;
        if (pic>=0 && !VWB_DrawPic(nx,ny,pic)) return false;
        nx+=advance;
    }
    return true;
}
static bool WriteNumber(int right,int y,unsigned value)
{
    char text[16];(void)snprintf(text,sizeof(text),"%u",value);
    return Write(right-(int)strlen(text)*2,y,text);
}
bool DrawLevelCompleted(const levelratio_t *r)
{
    VWB_Bar(0,0,320,160,127);
    if (!VWB_DrawPic(0,16,L_GUYPIC)) return false;
    if (gamestate.mapon>=8) return Write(14,4,"secret floor\n completed!") && Write(10,16,"15000 bonus!") && DrawStatus();
    char time[16],par[16];
    (void)snprintf(time,sizeof(time),"%02u:%02u",r->seconds/60,r->seconds%60);
    (void)snprintf(par,sizeof(par),"%02u:%02u",r->par/60,r->par%60);
    return Write(14,2,"floor\ncompleted") && WriteNumber(28,2,(unsigned)gamestate.mapon+1) &&
        Write(14,7,"bonus") && WriteNumber(36,7,(unsigned)r->bonus) &&
        Write(16,10,"time") && Write(26,10,time) && Write(16,12,"par") && Write(26,12,par) &&
        Write(9,14,"kill ratio") && Write(5,16,"secret ratio") && Write(1,18,"treasure ratio") &&
        WriteNumber(37,14,r->kill) && WriteNumber(37,16,r->secret) && WriteNumber(37,18,r->treasure) &&
        Write(37,14,"%") && Write(37,16,"%") && Write(37,18,"%") && DrawStatus();
}
