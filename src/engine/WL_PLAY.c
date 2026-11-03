/* SPDX-License-Identifier: GPL-2.0-only */
/* WL_PLAY.C controls and clock: normal play uses one 70-Hz tic per update;
   original demo commands use four-tic frames. Presentation is independent. */
#include "WL_DEF.h"
#include "WL_PRESENT.h"
#include "WL_DEMO.h"
#include "WL_SAVE.h"
#include "WL_CONTROLS.h"
#include "ID_IN.h"
#include "ID_CA.h"
#include "ID_SD.h"
#include "ID_VH.h"
#include "ID_US_1.h"
#include <math.h>
static uint64_t lasttimecount;
static bool running,finished,intermission;
static unsigned victorypage;
static bool preload_pending;
bool GamePreloadPending(void) { return preload_pending; }
void GamePreloadConsumed(void) { preload_pending=false; }
bool GameVictoryPresentation(void) { return victorypage!=0; }
bool GameIntermission(void) { return intermission; }
static unsigned deathphase,deathwait;
static int deathangle;
static uint32_t fizzlernd;
unsigned GameDeathPresentation(uint32_t *seed)
{
    if(seed) *seed=fizzlernd;
    return running && gamestate.died?deathphase:0;
}
bool GameFinished(void) { return finished; }
void DrawPlayBorder(void)
{
    VWB_Bar(0,0,320,160,127);
    VWB_Bar(VIEWX,VIEWY,VIEWWIDTH,VIEWHEIGHT,0);
    VWB_Hlin(VIEWX-1,VIEWX+VIEWWIDTH,VIEWY-1,0);
    VWB_Hlin(VIEWX-1,VIEWX+VIEWWIDTH,VIEWY+VIEWHEIGHT,125);
    VWB_Vlin(VIEWY-1,VIEWY+VIEWHEIGHT,VIEWX-1,0);
    VWB_Vlin(VIEWY-1,VIEWY+VIEWHEIGHT,VIEWX+VIEWWIDTH,125);
    framebuffer[(VIEWY+VIEWHEIGHT)*320+VIEWX-1]=124;
}
static bool DrawPlayScreen(void)
{
    DrawPlayBorder();return VWB_DrawPic(0,160,STATUSBARPIC) && DrawStatus();
}
bool GameProbe(void)
{
    US_InitRndT(0);
    bool valid=NewGame(2,0) && SetupGameLevel() && DrawPlayScreen() && ThreeDRefresh();
    ShutdownGameLevel(); return valid;
}
bool GameStart(uint64_t ticks) { return GameStartSelected(ticks,0,2); }
bool GameStartSelected(uint64_t ticks,unsigned episode,unsigned difficulty)
{
    if(episode>5) { GameStop();return CA_Fail("Invalid episode/difficulty"); }
    return GameStartMap(ticks,episode*10,difficulty,(unsigned)(ticks&255U));
}
bool GameStartMap(uint64_t ticks,unsigned map,unsigned difficulty,unsigned seed)
{
    GameStop();
    if(map>=60) return CA_Fail("Invalid original map number");
    if(!NewGame(difficulty,map/10)) return false;
    gamestate.mapon=(int)(map%10);US_InitRndT(seed);
    if (!SetupGameLevel() || !DrawPlayScreen() || !ThreeDRefresh()) { GameStop(); return false; }
    SD_StartMusic(SD_LevelMusic(map/10,map%10));
    deathphase=deathwait=0;finished=intermission=false;
    preload_pending=true;lasttimecount=ticks;running=true; IN_ClearKeysDown();WL_PresentCapture(ticks);return true;
}
/* WL_DEBUG.C Tab-W returns ex_warped to GameLoop: preserve inventory and
   episode, restore entry score, then perform the ordinary level setup. */
bool GameWarp(uint64_t ticks,unsigned floor)
{
    if(!GameCanSave() || floor<1 || floor>10) return CA_Fail("Invalid debug warp");
    ShutdownGameLevel();gamestate.mapon=(int)floor-1;gamestate.score=gamestate.oldscore;
    US_InitRndT((unsigned)(ticks&255U));
    if(!SetupGameLevel() || !DrawPlayScreen() || !ThreeDRefresh()) { GameStop();return false; }
    SD_StartMusic(SD_LevelMusic((unsigned)gamestate.episode,(unsigned)gamestate.mapon));
    preload_pending=true;deathphase=deathwait=0;lasttimecount=ticks;IN_ClearKeysDown();WL_PresentReset();WL_PresentCapture(ticks);return true;
}
void GameStop(void) { preload_pending=false;ResetPreloadGraphics(); WL_PresentReset(); Demo_Stop();CT_SetGameplay(false);SD_StopSound();SD_StartMusic(-1); victorypage=0;ResetEndText(); FinishPaletteShifts();running=intermission=false; ShutdownGameLevel(); }
/* WL_GAME.C Died and ID_VH.C FizzleFade, resumed one original tic at a
   time instead of blocking the SDL event loop. The 17-bit polynomial and
   64000/70 candidate pixels per tic are the original values. */
static bool Died(uint64_t ticks)
{
    if (!deathphase) {
        gamestate.weapon=-1;
        SD_StartMusic(-1);GameSound(PLAYERDEATHSND,NULL);
        if (killerobj) {
            float angle=(float)atan2((double)player.y-killerobj->y,(double)killerobj->x-player.x);
            if (angle<0) angle=(float)(6.28318530717958647692+angle);
            deathangle=(int)(angle/6.28318530717958647692*ANGLES)%ANGLES;
        } else deathangle=player.angle;
        deathphase=1;
    }
    if (deathphase==1) {
        /* DOS clockwise<counter: equal 180-degree arcs turn negatively.
           GameStep resumes this once per elapsed 70-Hz tic (2*tics total). */
        int delta=(deathangle-player.angle+540)%360-180;
        int change=delta>2 ? 2 : delta< -2 ? -2 : delta;
        player.angle=(player.angle+change+360)%360;
        if (!ThreeDRefresh() || !DrawStatus()) return false;
        if (player.angle==deathangle) { FinishPaletteShifts();deathphase=2;fizzlernd=1;IN_ClearKeysDown(); }
    } else if (deathphase==2) {
        for (unsigned p=0;p<64000/70;++p) {
            unsigned x=fizzlernd>>8,y=(fizzlernd-1)&255U;
            fizzlernd=(fizzlernd>>1)^((fizzlernd&1U)?UINT32_C(0x12000):0U);
            /* Original x>width/y>height allowed border writes. Clip to the
               actual viewport here, preserving its surrounding bevel. */
            if (x<VIEWWIDTH && y<VIEWHEIGHT) framebuffer[(VIEWY+y)*320+VIEWX+x]=4;
            if (fizzlernd==1) { deathphase=3;deathwait=100;IN_ClearKeysDown();break; }
        }
    } else if (deathphase==3 && (!--deathwait || IN_AnyPressed())) {
        int lives=gamestate.lives-1,nextextra=gamestate.nextextra;
        gamestate.lives=lives;
        if (lives<0) { finished=true;return true; }
        /* WL_GAME.C Died/GameLoop: entry score, fresh pistol, same floor. */
        gamestate.score=gamestate.oldscore;
        gamestate.health=100;gamestate.ammo=8;gamestate.keys=0;
        gamestate.weapon=gamestate.bestweapon=gamestate.chosenweapon=wp_pistol;
        ShutdownGameLevel();
        US_InitRndT((unsigned)(ticks&255U));
        if (!SetupGameLevel()) return false;
        gamestate.lives=lives;gamestate.nextextra=nextextra;
        SD_StartMusic(SD_LevelMusic((unsigned)gamestate.episode,(unsigned)gamestate.mapon));
        deathphase=0;IN_ClearKeysDown();
        if (!DrawPlayScreen() || !ThreeDRefresh()) return false;
    }
    return true;
}
bool GameStep(uint64_t ticks)
{
    if (!running || ticks<lasttimecount) return CA_Fail("Invalid simulation clock/state");
    uint64_t elapsed=ticks-lasttimecount;
    /* Headless/core callers may step directly; the interactive frontend
       consumes this request before advancing the simulation. */
    preload_pending=false;
    if (!elapsed) return true; /* Retain quick make/break input until a tic runs. */
    unsigned dt=(unsigned)SimulationTics();
    if(Demo_Active()) {
        if(elapsed<4) return true;
        uint64_t remainder=elapsed%4;lasttimecount=ticks-remainder;elapsed-=remainder;
        if(elapsed>8) elapsed=8;
    } else lasttimecount=ticks;
    if (elapsed>MAXTICS) elapsed=MAXTICS; /* Discard long-pause excess, as CalcTics. */
    if (intermission) {
        bool leave=false;
        if(!StepLevelIntermission(ticks,IN_AnyPressed(),&leave)) return false;
        if(!leave) return true;
        unsigned next=NextMap((unsigned)gamestate.episode,(unsigned)gamestate.mapon,playstate);
        if (next>=60 || next/10!=(unsigned)gamestate.episode) return CA_Fail("Invalid campaign progression");
        gamestate.oldscore=gamestate.score;gamestate.mapon=(int)(next%10);
        ShutdownGameLevel();
        if (!SetupGameLevel() || !DrawPlayScreen() || !ThreeDRefresh()) return false;
        SD_StartMusic(SD_LevelMusic((unsigned)gamestate.episode,(unsigned)gamestate.mapon));
        preload_pending=true;intermission=false;IN_ClearKeysDown();WL_PresentCapture(ticks);return true;
    }
    if (victorypage) {
        if (victorypage==1) {
            if (IN_AnyPressed()) {
                IN_ClearKeysDown();if (!StartEndText()) return false;victorypage=2;
            }
        } else {
            bool done=false;if (!StepEndText(&done)) return false;
            if (done) { finished=true;ResetEndText(); }
        }
        return true;
    }
    for (uint64_t tic=0;tic<elapsed;tic+=dt) {
        if (finished) break;
        demo_command demo={0};bool playback=Demo_Active();
        if(playback && !Demo_ReadTic(&demo)) break;
        if (gamestate.died) { if (!Died(ticks-elapsed+tic+1)) return false;WL_PresentCapture(ticks-elapsed+tic+dt);continue; }
        if (DeathCameraActive()) { if (!StepDeathCamera()) return false;WL_PresentCapture(ticks-elapsed+tic+dt);continue; }
        if (gamestate.TimeCount<=UINT32_MAX-dt) gamestate.TimeCount+=dt;
        /* Original T_Player returns before face/input in BJ victory; T_Attack
           still updates the face, while s_deathcam has no player thinker. */
        if (!gamestate.victoryflag || (gamestate.attackframe && player.state!=&s_deathcam)) UpdateFace();
        thrustspeed=0;
        MoveDoors();MovePWalls();
        if (gamestate.attackframe && playback && demo.use) Demo_SuppressButtons(8U);
        if (!gamestate.attackframe && (playback?demo.use:CT_Pressed(CT_USE)) && !gamestate.victoryflag) Cmd_Use();
        if (playstate==ex_victorious || (playstate==ex_completed && gamestate.mapon==8)) {
            SD_StartMusic(24);
            FinishPaletteShifts();victorypage=1;IN_ClearKeysDown();return DrawVictory();
        }
        if (playstate!=ex_playing) {
            if (gamestate.mapon==8) return CA_Fail("Invalid secret exit on episode finale");
            gamestate.keys=0;FinishPaletteShifts();
            levelratio_t stats=LevelStats();
            SD_StartMusic(16);
            intermission=true;IN_ClearKeysDown();
            return StartLevelIntermission(&stats,ticks);
        }
        int move=CT_Down(CT_RUN)?70:35;
        int turn=(CT_Down(CT_RIGHT)?1:0)-(CT_Down(CT_LEFT)?1:0);
        int forward=(CT_Down(CT_FORWARD)?1:0)-(CT_Down(CT_BACK)?1:0);
        int strafe=(CT_Down(CT_STRAFE_RIGHT)?1:0)-(CT_Down(CT_STRAFE_LEFT)?1:0);
        bool alt=CT_Down(CT_ALT);int mouse_turn=CT_MouseTurn();
        if (gamestate.victoryflag) {
            if (player.state==&s_deathcam) ++player.temp1;
            else VictorySpin();
        } else if(playback) {
            ControlMovement(demo.x*(int)dt,demo.y*(int)dt,(demo.buttons&2U)!=0);
        } else if(CT_Preset()==CT_CLASSIC && !strafe) {
            ControlMovement(turn*move,-forward*move,alt);
            if(mouse_turn) ControlMovement(mouse_turn,0,false);
        } else {
            if(alt) { strafe+=turn;turn=0;if(strafe>1) strafe=1;if(strafe< -1) strafe=-1; }
            ModernMovement(turn*move+mouse_turn,forward,strafe,move);
        }
        ConnectAreas();
        if (!gamestate.victoryflag) T_Attack(playback?(demo.buttons&1U)!=0:CT_Down(CT_FIRE));
        for (objtype *ob=player.next;ob;ob=ob->next) {
            DoActor(ob);if (DeathCameraActive()) break;
        }
        if (ActorAllocationFailed()) return false;
        UpdateSoundLoc();
        madenoise=false;
        IN_ClearPressed();CT_EndTic();
        /* A_StartDeathCam's original blocking hold begins before the outer
           palette update. Keep that ordering while its wait is resumed. */
        if ((!DeathCameraActive() && !UpdatePaletteShifts()) || !ThreeDRefresh() || !DrawStatus()) return false;
        WL_PresentCapture(ticks-elapsed+tic+dt);
    }
    return true;
}

bool GameRunning(void) { return running && !finished; }
bool GameCanSave(void)
{
    return GameRunning() && !GameLoading() && !Demo_Active() && CA_Map() && !intermission && !victorypage && !gamestate.died &&
        !gamestate.victoryflag && !DeathCameraActive() && playstate==ex_playing && !ActorAllocationFailed();
}
void GamePauseClock(uint64_t ticks) { lasttimecount=ticks;WL_PresentReset();WL_PresentCapture(ticks); }
void GameRestoreClock(uint64_t ticks)
{
    WL_PresentReset();running=true;finished=intermission=false;victorypage=0;deathphase=deathwait=0;
    preload_pending=true;lasttimecount=ticks;ResetVictory();ResetEndText();ClearPaletteShifts();WL_PresentCapture(ticks);
}
bool GameDrawPaused(void) { InvalidateStatus();return DrawPlayScreen() && ThreeDRefreshPaused(); }
