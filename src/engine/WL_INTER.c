/* SPDX-License-Identifier: GPL-2.0-only */
/* WL_INTER.C DrawHighScores: original default table from ID_US_1.C.
   Native scores keep original qualification and fixed-width number glyphs. */
#include "WL_MENU.h"
#include "ID_VH.h"
#include "ID_CA.h"
#include "ID_US_1.h"
#include "WL_HIGHSCORES.h"
#include <stdio.h>
bool DrawHighScores(void)
{
    const high_score *scores=HighScores_Table();
    ClearMScreen(); DrawStripes(10);
    if (!VWB_DrawPic(48,0,HIGHSCORESPIC) || !VWB_DrawPic(32,68,C_NAMEPIC) ||
        !VWB_DrawPic(160,68,C_LEVELPIC) || !VWB_DrawPic(224,68,C_SCOREPIC)) return false;
    fontnumber = 0; fontcolor = 15;
    for (int i = 0; i < 7; ++i) {
        int width, height;char level[16],score[16],episode[16];
        (void)snprintf(level,sizeof(level),"%u",scores[i].completed);
        (void)snprintf(score,sizeof(score),"%u",scores[i].score);
        (void)snprintf(episode,sizeof(episode),"E%u/L",scores[i].episode+1);
        for(char *p=level;*p;++p) *p=(char)((unsigned char)*p+129U-'0');
        for(char *p=score;*p;++p) *p=(char)((unsigned char)*p+129U-'0');
        PrintX = 32; PrintY = 76 + 16*i;
        if (!US_Print(scores[i].name) || !VW_MeasurePropString(level,&width,&height)) return false;
        PrintX = 22*8-width-6;
        if (!US_Print(episode) || !US_Print(level) || !VW_MeasurePropString(score,&width,&height)) return false;
        PrintX = 34*8-8-width;
        if (!US_Print(score)) return false;
    }
    return true;
}

/* WL_INTER.C LevelCompleted: original par times, integer ratios and bonuses.
   The counting/sound/breathing state machine below shares this result drawing. */
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
        else if (ch=='\'') { pic=L_APOSTROPHEPIC;advance=8; }
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

bool DrawDeathCameraMessage(void) { return Write(0,7,"Let's see that again!"); }

/* WL_INTER.C Victory: the original eight-floor averages, time and time code. */
bool DrawVictory(void)
{
    unsigned seconds=0,kill=0,secret=0,treasure=0;
    for (unsigned i=0;i<8;++i) {
        seconds+=LevelRatios[i].seconds;kill+=LevelRatios[i].kill;
        secret+=LevelRatios[i].secret;treasure+=LevelRatios[i].treasure;
    }
    unsigned minutes=seconds/60;seconds%=60;
    if (minutes>99) minutes=seconds=99;
    VWB_Bar(0,0,320,160,127);
    char time[16];(void)snprintf(time,sizeof(time),"%02u:%02u",minutes,seconds);
    if (!Write(18,2,"you win!") || !Write(14,6,"total time") || !Write(12,12,"averages") ||
        !Write(14,14,"kill    %") || !Write(10,16,"secret    %") || !Write(6,18,"treasure    %") ||
        !VWB_DrawPic(8,4,L_BJWINSPIC) || !Write(14,8,time) ||
        !WriteNumber(30,14,kill/8) || !WriteNumber(30,16,secret/8) ||
        !WriteNumber(30,18,treasure/8)) return false;
    if (gamestate.difficulty>=2) {
        if (!VWB_DrawPic(240,64,C_TIMECODEPIC)) return false;
        char code[4];code[0]=(char)(((minutes/10)^(minutes%10)^10)+'A');
        code[1]=(char)(((seconds/10)^(seconds%10)^10)+'A');
        code[2]=(char)((code[0]^code[1])+'A');code[3]=0;
        fontnumber=0;fontcolor=0x47;PrintX=241;PrintY=72;
        if (!US_Print(code)) return false;
    }
    return DrawStatus();
}

/* WL_INTER.C LevelCompleted loops and BJ_Breathe, resumed without DOS waits.
   Counters run until the original sound wait or 30-VBL boundary. A logical
   140-Hz effect length supplies the wait even with output muted/unavailable. */
#include "ID_SD.h"
#include "ID_IN.h"
static unsigned breath_frame,breath_max=10;
static struct {
    levelratio_t final,shown;
    unsigned stage,index,part,breath;
    uint64_t until,breath_next;
    bool complete,awarded;
} completion;
static bool Breathe(uint64_t ticks)
{
    if(ticks>=completion.breath_next) {
        uint64_t steps=1+(ticks-completion.breath_next)/36;
        completion.breath_next+=steps*36;breath_frame^=(unsigned)(steps&1U);breath_max=35;
    }
    completion.breath=breath_frame;
    return VWB_DrawPic(0,16,breath_frame?L_GUY2PIC:L_GUYPIC);
}
static bool CompleteResults(uint64_t ticks)
{
    completion.shown=completion.final;completion.complete=true;
    completion.breath_next=ticks+breath_max+1;
    if(!completion.awarded) {
        GivePoints(completion.final.bonus);completion.awarded=true;
        if(gamestate.mapon<8) LevelRatios[gamestate.mapon]=completion.final;
    }
    return DrawLevelCompleted(&completion.final) && VWB_DrawPic(0,16,breath_frame?L_GUY2PIC:L_GUYPIC);
}
static void CountSound(soundnames sound)
{
    GameSound(sound,NULL);completion.until+=SD_SoundTicks(sound);
}
bool LevelIntermissionCounting(void) { return !completion.complete; }
bool StartLevelIntermission(const levelratio_t *stats,uint64_t ticks)
{
    memset(&completion,0,sizeof(completion));completion.final=*stats;completion.shown=*stats;
    completion.shown.kill=completion.shown.secret=completion.shown.treasure=0;completion.shown.bonus=0;
    completion.until=ticks;completion.breath=breath_frame;completion.breath_next=ticks+breath_max+1;
    if(gamestate.mapon>=8) return CompleteResults(ticks);
    bool leave=false;return StepLevelIntermission(ticks,false,&leave);
}
bool StepLevelIntermission(uint64_t ticks,bool acknowledge,bool *leave)
{
    *leave=false;
    if(acknowledge) {
        if(completion.complete) { *leave=true;return true; }
        IN_ClearKeysDown();return CompleteResults(ticks);
    }
    if(completion.complete) return Breathe(ticks);
    /* Maximum legitimate WL6 count is bounded by the original map totals.
       Run instant iterations together; never invent a frame-rate counter. */
    for(unsigned work=0;work<20000 && !completion.complete && ticks>=completion.until;++work) {
        unsigned limit=completion.stage==0 ? (completion.final.seconds<completion.final.par?completion.final.par-completion.final.seconds:0) :
            completion.stage==1?completion.final.kill:completion.stage==2?completion.final.secret:completion.final.treasure;
        if(completion.part==0) {
            if(completion.stage==0 && !limit) { completion.stage=1;continue; }
            if(completion.index<=limit) {
                if(completion.stage==0) completion.shown.bonus=(int)completion.index*500;
                else if(completion.stage==1) completion.shown.kill=completion.index;
                else if(completion.stage==2) completion.shown.secret=completion.index;
                else completion.shown.treasure=completion.index;
                unsigned interval=completion.stage==0?50U:10U;
                if(completion.index%interval==0) CountSound(ENDBONUS1SND);
                ++completion.index;
            } else completion.part=1;
        } else if(completion.part==1) {
            if(completion.stage && (limit==100 || !limit)) { completion.until+=30;completion.part=2; }
            else { CountSound(ENDBONUS2SND);completion.part=3; }
        } else if(completion.part==2) {
            SD_StopSound();
            if(limit==100) { completion.shown.bonus+=10000;CountSound(PERCENT100SND); }
            else CountSound(NOBONUSSND);
            completion.part=3;
        } else {
            ++completion.stage;completion.index=completion.part=0;
            if(completion.stage==4) { IN_ClearKeysDown();if(!CompleteResults(ticks)) return false; }
        }
    }
    if(!completion.complete && !DrawLevelCompleted(&completion.shown)) return false;
    if(!completion.complete) {
        for(unsigned stage=1;stage<=3;++stage) if(completion.stage<stage)
            VWB_Bar(29*8,(12+(int)stage*2)*8,64,16,127);
        if(!Breathe(ticks)) return false;
    }
    return true;
}

/* WL_INTER.C PreloadGraphics/PreloadUpdate, resumed by the frontend so SDL
   keeps polling events. Native heap caching replaces DOS main/EMS/XMS limits.
   Count only present, uncached VSWAP pages, just as PM_Preload does. */
#include "ID_PM.h"
#include "WL_SAVE.h"
static struct {
    unsigned phase,page,current,total;
    uint64_t until;
} preload;
bool GameLoading(void) { return preload.phase!=0; }
void ResetPreloadGraphics(void) { memset(&preload,0,sizeof(preload)); }
void PreloadUpdate(unsigned current,unsigned total)
{
    unsigned w=total?(unsigned)(UINT64_C(214)*current/total):0;
    VWB_Bar(53,101,214,2,0);
    if(w) {
        VWB_Bar(53,101,(int)w,2,0x37);
        VWB_Bar(53,101,(int)w-1,1,0x32);
    }
}
bool StartPreloadGraphics(uint64_t ticks)
{
    ResetPreloadGraphics();GamePreloadConsumed();
    VWB_Bar(0,0,320,160,127);
    if(!VWB_DrawPic(48,56,GETPSYCHEDPIC)) return false;
    for(unsigned p=0;p<ChunksInFile;++p)
        if(PM_PagePresent(p) && !PM_PageCached(p)) ++preload.total;
    preload.phase=1;preload.until=ticks+30;IN_ClearKeysDown();
    return VL_FadeOut(0,0,0,0,ticks) && VL_FadeIn(gamepal,30,ticks);
}
bool StepPreloadGraphics(uint64_t ticks)
{
    VL_FadeStep(ticks);
    if(preload.phase==1) {
        if(ticks<preload.until) return true;
        preload.phase=2;
    }
    if(preload.phase==2) {
        /* At most 32 actual page reads per frontend update. No invented
           percentage or artificial minimum loading duration. */
        unsigned loaded=0;
        while(preload.page<ChunksInFile && loaded<32) {
            unsigned p=preload.page++;
            if(!PM_PagePresent(p) || PM_PageCached(p)) continue;
            if(!PM_GetPage(p,NULL)) return false;
            ++loaded;++preload.current;PreloadUpdate(preload.current,preload.total);
        }
        if(preload.page<ChunksInFile) return true;
        preload.phase=3;preload.until=ticks+70;IN_ClearKeysDown();
    }
    if(preload.phase==3) {
        if(ticks<preload.until && !IN_AnyPressed()) return true;
        IN_ClearKeysDown();preload.phase=4;preload.until=ticks+30;
        return VL_FadeOut(0,0,0,30,ticks);
    }
    if(preload.phase==4 && ticks>=preload.until) {
        /* DOS restores the border while black before revealing the level. */
        if(!GameDrawPaused()) return false;
        preload.phase=5;preload.until=ticks+30;
        return VL_FadeIn(gamepal,30,ticks);
    }
    if(preload.phase==5 && ticks>=preload.until) {
        ResetPreloadGraphics();IN_ClearKeysDown();GamePauseClock(ticks);
    }
    return true;
}
