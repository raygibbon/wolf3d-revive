/* SPDX-License-Identifier: GPL-2.0-only */
/* WL_PLAY.C keyboard controls driving the original one-tic ControlMovement.
   Simulation is explicitly 70 Hz; presentation is independent. */
#include "WL_DEF.h"
#include "ID_IN.h"
#include "ID_CA.h"
#include "ID_VH.h"
#include "ID_US_1.h"
#include <math.h>
static uint64_t lasttimecount;
static bool running,finished,intermission;
static unsigned victorypage;
bool GameVictoryPresentation(void) { return victorypage!=0; }
bool GameIntermission(void) { return intermission; }
static unsigned deathphase,deathwait;
static int deathangle;
static uint32_t fizzlernd;
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
    GameStop();
    US_InitRndT((unsigned)(ticks&255U));
    if (!NewGame(difficulty,episode) || !SetupGameLevel() || !DrawPlayScreen() || !ThreeDRefresh()) { GameStop(); return false; }
    deathphase=deathwait=0;finished=intermission=false;
    lasttimecount=ticks;running=true; IN_ClearKeysDown(); return true;
}
void GameStop(void) { victorypage=0;ResetEndText(); FinishPaletteShifts();running=intermission=false; ShutdownGameLevel(); }
static bool Down(unsigned scan) { return Keyboard[scan] || IN_PressPending(scan); }
/* WL_GAME.C Died and ID_VH.C FizzleFade, resumed one original tic at a
   time instead of blocking the SDL event loop. The 17-bit polynomial and
   64000/70 candidate pixels per tic are the original values. */
static bool Died(uint64_t ticks)
{
    if (!deathphase) {
        if (killerobj) {
            double angle=atan2((double)player.y-killerobj->y,(double)killerobj->x-player.x);
            if (angle<0) angle+=6.28318530717958647692;
            deathangle=(int)(angle/6.28318530717958647692*ANGLES);
        } else deathangle=player.angle;
        deathphase=1;
    }
    if (deathphase==1) {
        int delta=(deathangle-player.angle+540)%360-180;
        int change=delta>2 ? 2 : delta< -2 ? -2 : delta;
        player.angle=(player.angle+change+360)%360;
        if (!ThreeDRefresh() || !DrawStatus()) return false;
        if (!delta) { FinishPaletteShifts();deathphase=2;fizzlernd=1;IN_ClearKeysDown(); }
    } else if (deathphase==2) {
        for (unsigned p=0;p<64000/70;++p) {
            unsigned x=fizzlernd>>8,y=(fizzlernd-1)&255U;
            fizzlernd=(fizzlernd>>1)^((fizzlernd&1U)?UINT32_C(0x12000):0U);
            /* Original x>width/y>height allowed border writes. Clip to the
               actual viewport here, preserving its surrounding bevel. */
            if (x<VIEWWIDTH && y<VIEWHEIGHT) framebuffer[(VIEWY+y)*320+VIEWX+x]=4;
            if (fizzlernd==1) { deathphase=3;deathwait=100;break; }
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
        deathphase=0;IN_ClearKeysDown();
        if (!DrawPlayScreen() || !ThreeDRefresh()) return false;
    }
    return true;
}
bool GameStep(uint64_t ticks)
{
    if (!running || ticks<lasttimecount) return CA_Fail("Invalid simulation clock/state");
    uint64_t elapsed=ticks-lasttimecount;
    if (!elapsed) return true; /* Retain quick make/break input until a tic runs. */
    lasttimecount=ticks;
    if (elapsed>MAXTICS) elapsed=MAXTICS; /* Discard long-pause excess, as CalcTics. */
    if (intermission) {
        if (!IN_AnyPressed()) return true;
        unsigned next=NextMap((unsigned)gamestate.episode,(unsigned)gamestate.mapon,playstate);
        if (next>=60 || next/10!=(unsigned)gamestate.episode) return CA_Fail("Invalid campaign progression");
        gamestate.oldscore=gamestate.score;gamestate.mapon=(int)(next%10);
        ShutdownGameLevel();
        if (!SetupGameLevel() || !DrawPlayScreen() || !ThreeDRefresh()) return false;
        intermission=false;IN_ClearKeysDown();return true;
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
    for (uint64_t tic=0;tic<elapsed;++tic) {
        if (finished) break;
        if (gamestate.died) { if (!Died(ticks-elapsed+tic+1)) return false;continue; }
        if (DeathCameraActive()) { if (!StepDeathCamera()) return false;continue; }
        if (gamestate.TimeCount<UINT32_MAX) ++gamestate.TimeCount;
        /* Original T_Player returns before face/input in BJ victory; T_Attack
           still updates the face, while s_deathcam has no player thinker. */
        if (!gamestate.victoryflag || (gamestate.attackframe && player.state!=&s_deathcam)) UpdateFace();
        thrustspeed=0;
        MoveDoors();MovePWalls();
        if (IN_ConsumePress(sc_Space) && !gamestate.victoryflag) Cmd_Use();
        if (playstate==ex_victorious || (playstate==ex_completed && gamestate.mapon==8)) {
            FinishPaletteShifts();victorypage=1;IN_ClearKeysDown();return DrawVictory();
        }
        if (playstate!=ex_playing) {
            if (gamestate.mapon==8) return CA_Fail("Invalid secret exit on episode finale");
            gamestate.keys=0;FinishPaletteShifts();
            levelratio_t stats=LevelStats();
            if (gamestate.mapon<8) LevelRatios[gamestate.mapon]=stats;
            GivePoints(stats.bonus);intermission=true;IN_ClearKeysDown();
            return DrawLevelCompleted(&stats);
        }
        int move=(Down(0x2a) || Down(0x36)) ? 70 : 35;
        int cx=(Down(sc_RightArrow) ? move : 0)-(Down(sc_LeftArrow) ? move : 0);
        int cy=(Down(sc_DownArrow) ? move : 0)-(Down(sc_UpArrow) ? move : 0);
        if (gamestate.victoryflag) {
            if (player.state==&s_deathcam) ++player.temp1;
            else VictorySpin();
        }
        else ControlMovement(cx,cy,Down(0x38));
        ConnectAreas();
        if (!gamestate.victoryflag) T_Attack(Down(0x1d));
        for (objtype *ob=player.next;ob;ob=ob->next) {
            DoActor(ob);if (DeathCameraActive()) break;
        }
        if (ActorAllocationFailed()) return false;
        madenoise=false;
        IN_ClearPressed();
        /* A_StartDeathCam's original blocking hold begins before the outer
           palette update. Keep that ordering while its wait is resumed. */
        if ((!DeathCameraActive() && !UpdatePaletteShifts()) || !ThreeDRefresh() || !DrawStatus()) return false;
    }
    return true;
}
