/* SPDX-License-Identifier: GPL-2.0-only */
/* id Software WL_ACT2.C BJ victory / A_StartDeathCam and
   WL_AGENT.C VictoryTile/VictorySpin, resumed in the existing 70 Hz loop.
   Original source/licensing: WOLFSRC/ and docs/LICENSING.md. */
#include "WL_DEF.h"
#include "ID_CA.h"
#include "ID_IN.h"
#include "ID_VH.h"
#include "ID_US_1.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
#define BJRUNSPEED 2048
#define BJJUMPSPEED 680
static objtype *camactor;
static unsigned camphase,camwait;
static uint32_t camrnd;
static w3r_byte replay[320*160];
static void T_BJRun(objtype *ob);
static void T_BJJump(objtype *ob);
static void T_BJDone(objtype *ob);
static void T_BJYell(objtype *ob);
extern statetype s_bjrun1,s_bjrun1s,s_bjrun2,s_bjrun3,s_bjrun3s,s_bjrun4;
extern statetype s_bjjump1,s_bjjump2,s_bjjump3,s_bjjump4;
statetype s_deathcam = {false,0,0,NULL,NULL,NULL};
statetype s_bjrun1 = {false,SPR_BJ_W1,12,T_BJRun,NULL,&s_bjrun1s};
statetype s_bjrun1s = {false,SPR_BJ_W1,3,NULL,NULL,&s_bjrun2};
statetype s_bjrun2 = {false,SPR_BJ_W2,8,T_BJRun,NULL,&s_bjrun3};
statetype s_bjrun3 = {false,SPR_BJ_W3,12,T_BJRun,NULL,&s_bjrun3s};
statetype s_bjrun3s = {false,SPR_BJ_W3,3,NULL,NULL,&s_bjrun4};
statetype s_bjrun4 = {false,SPR_BJ_W4,8,T_BJRun,NULL,&s_bjrun1};
statetype s_bjjump1 = {false,SPR_BJ_JUMP1,14,T_BJJump,NULL,&s_bjjump2};
statetype s_bjjump2 = {false,SPR_BJ_JUMP2,14,T_BJJump,T_BJYell,&s_bjjump3};
statetype s_bjjump3 = {false,SPR_BJ_JUMP3,14,T_BJJump,NULL,&s_bjjump4};
statetype s_bjjump4 = {false,SPR_BJ_JUMP4,300,NULL,T_BJDone,&s_bjjump4};
void ResetVictory(void) { camactor=NULL;camphase=camwait=0; }
bool DeathCameraActive(void) { return camphase!=0; }
bool VictoryTile(void)
{
    if (gamestate.victoryflag) return true;
    objtype *ob=SpawnActor(player.tilex,player.tiley+1,&s_bjrun1);
    if (!ob) return false;
    ob->x=player.x;ob->y=player.y;ob->obclass=bjobj;
    ob->dir=north;ob->temp1=6;gamestate.victoryflag=true;return true;
}
void VictorySpin(void)
{
    if (player.angle>270) { player.angle-=3;if (player.angle<270) player.angle=270; }
    else if (player.angle<270) { player.angle+=3;if (player.angle>270) player.angle=270; }
    int32_t dest=(player.tiley-5)*TILEGLOBAL-0x3000;
    /* Invalid victory routes cannot drive the camera out of the map. */
    if (dest<MINDIST) dest=MINDIST;
    if (player.y>dest) { player.y-=4096;if (player.y<dest) player.y=dest; }
}
static void T_BJRun(objtype *ob)
{
    int32_t move=BJRUNSPEED;
    while (move) {
        if (move<ob->distance) { MoveObj(ob,move);break; }
        ob->x=ob->tilex*TILEGLOBAL+TILEGLOBAL/2;
        ob->y=ob->tiley*TILEGLOBAL+TILEGLOBAL/2;
        move-=ob->distance;SelectPathDir(ob);
        if (!--ob->temp1) { NewState(ob,&s_bjjump1);return; }
        if (ob->dir==nodir) return;
    }
}
static void T_BJJump(objtype *ob) { MoveObj(ob,BJJUMPSPEED); }
static void T_BJYell(objtype *ob) { GameSound(YEAHSND,ob); }
static void T_BJDone(objtype *ob) { (void)ob;playstate=ex_victorious; }
void A_StartDeathCam(objtype *ob)
{
    if (camphase || gamestate.died) return;
    FinishPaletteShifts();camactor=ob;camphase=gamestate.victoryflag ? 4U : 1U;camwait=100;
}
/* Original CheckPosition ignores live actors, but tests the complete physical
   camera square against wall/static sentinels. Bounds replace DOS overreads. */
static bool CameraPosition(void)
{
    int xl=WL_FloorShift((int64_t)player.x-MINDIST,16),xh=WL_FloorShift((int64_t)player.x+MINDIST,16);
    int yl=WL_FloorShift((int64_t)player.y-MINDIST,16),yh=WL_FloorShift((int64_t)player.y+MINDIST,16);
    if (xl<0 || yl<0 || xh>=64 || yh>=64) return false;
    for (int x=xl;x<=xh;++x) for (int y=yl;y<=yh;++y) {
        unsigned tile=tilemap[x][y];
        if (staticblock[x][y] || (tile && tile!=64 && (!(tile&128) || (tile&64) || DoorSolid(tile&63)))) return false;
    }
    return true;
}
bool StepDeathCamera(void)
{
    if (!camphase) return true;
    if (camphase==5) {
        /* WL_DRAW.C fizzlein: 20-frame budget, restricted to the viewport. */
        for (unsigned i=0;i<64000/20;++i) {
            unsigned x=camrnd>>8,y=(camrnd-1)&255U;
            camrnd=(camrnd>>1)^((camrnd&1U)?UINT32_C(0x12000):0U);
            if (x<VIEWWIDTH && y<VIEWHEIGHT) {
                unsigned pixel=(VIEWY+y)*320+VIEWX+x;framebuffer[pixel]=replay[pixel];
            }
            if (camrnd==1) { camphase=0;break; }
        }
        return true;
    }
    if (camphase==4) {
        if (!--camwait) { playstate=ex_victorious;camphase=0; }
        return true;
    }
    if (camphase==1) {
        if (--camwait) return true;
        gamestate.victoryflag=true;camphase=2;camrnd=1;
        IN_ClearKeysDown();return true;
    }
    if (camphase==2) {
        /* ID_VH.C FizzleFade: full top play screen, 70 Hz candidate budget. */
        for (unsigned i=0;i<64000/70;++i) {
            unsigned x=camrnd>>8,y=(camrnd-1)&255U;
            camrnd=(camrnd>>1)^((camrnd&1U)?UINT32_C(0x12000):0U);
            if (x<320 && y<160) framebuffer[y*320+x]=127;
            if (camrnd==1) {
                camphase=3;camwait=300;IN_ClearKeysDown();
                return DrawDeathCameraMessage();
            }
        }
        return true;
    }
    if (--camwait && !IN_AnyPressed()) return true;
    float angle=(float)atan2((double)gamestate.killy-camactor->y,(double)camactor->x-gamestate.killx);
    if (angle<0) angle=(float)(angle+6.28318530717958647692);
    player.angle=((int)(angle/6.28318530717958647692*360))%ANGLES;
    bool found=false;
    for (int32_t dist=0x14000;dist<64*TILEGLOBAL;dist+=0x1000) {
        player.x=camactor->x-FixedByFrac(dist,sintable[player.angle+90]);
        player.y=camactor->y+FixedByFrac(dist,sintable[player.angle]);
        if (CameraPosition()) { found=true;break; }
    }
    if (!found) return CA_Fail("No valid boss death-camera position");
    player.tilex=player.x/TILEGLOBAL;player.tiley=player.y/TILEGLOBAL;player.state=&s_deathcam;player.temp1=0;
    switch (camactor->obclass) {
    case schabbobj:NewState(camactor,&s_schabbdeathcam);break;
    case giftobj:NewState(camactor,&s_giftdeathcam);break;
    case fatobj:NewState(camactor,&s_fatdeathcam);break;
    case realhitlerobj:NewState(camactor,&s_hitlerdeathcam);break;
    default:return CA_Fail("Invalid death-camera actor");
    }
    IN_ClearKeysDown();memcpy(replay,framebuffer,sizeof(replay));
    DrawPlayBorder();if (!UpdatePaletteShifts() || !ThreeDRefresh()) return false;
    for (unsigned y=0;y<VIEWHEIGHT;++y) for (unsigned x=0;x<VIEWWIDTH;++x) {
        unsigned i=(VIEWY+y)*320+VIEWX+x;
        w3r_byte pixel=framebuffer[i];framebuffer[i]=replay[i];replay[i]=pixel;
    }
    camrnd=1;camphase=5;return true;
}
