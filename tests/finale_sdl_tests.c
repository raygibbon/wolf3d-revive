/* SPDX-License-Identifier: GPL-2.0-only */
/* Controlled original-weapon fights and finale flows, not navigated episodes.
   Owned files are read in place. Positions, ammo and health are controlled. */
#include "engine/WL_DEF.h"
#include "engine/WL_MAIN.h"
#include "engine/ID_CA.h"
#include "engine/ID_IN.h"
#include "platform/sdl_video.h"
#include <SDL3/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#define CHECK(x) do { if (!(x)) { fprintf(stderr,"%d: %s (%s; %s)\n",__LINE__,#x,CA_Error(),VL_Error());exit(1); } } while (0)
static uint64_t ticks;
static unsigned ep;
static void Step(void)
{
    CHECK(!VL_PollQuit());CHECK(DemoLoop(++ticks));CHECK(VW_UpdateScreen());
    CHECK(DemoLoopState()!=UI_ERROR);
}
static void Steps(unsigned count) { while (count--) Step(); }
static void Key(SDL_Scancode scan,bool down)
{
    SDL_Event e={0};e.type=down ? SDL_EVENT_KEY_DOWN : SDL_EVENT_KEY_UP;e.key.scancode=scan;
    CHECK(SDL_PushEvent(&e));
}
static void Tap(SDL_Scancode scan) { Key(scan,true);Key(scan,false);Step(); }
static objtype *Find(int kind)
{
    for (objtype *ob=player.next;ob;ob=ob->next)
        if (ob->obclass==kind && (ob->flags&FL_SHOOTABLE)) return ob;
    return NULL;
}
static void Aim(objtype *ob)
{
    double a=atan2((double)player.y-ob->y,(double)ob->x-player.x);
    if (a<0) a+=6.28318530717958647692;
    player.angle=(int)(a*360/6.28318530717958647692);
}
static void Arrange(objtype *ob)
{
    for (unsigned i=0;i<doornum;++i)
        if (abs((int)doorobjlist[i].x-ob->tilex)<10 && abs((int)doorobjlist[i].y-ob->tiley)<10) OpenDoor(i);
    for (unsigned i=0;i<70;++i) MoveDoors();
    bool found=false;
    for (int d=2;d<10 && !found;++d) for (int dy=-d;dy<=d && !found;++dy)
        for (int dx=-d;dx<=d && !found;++dx) {
            if (abs(dx)!=d && abs(dy)!=d) continue;
            int x=ob->tilex+dx,y=ob->tiley+dy;if (x<1 || y<1 || x>62 || y>62) continue;
            player.x=x*TILEGLOBAL+32768;player.y=y*TILEGLOBAL+32768;
            player.tilex=x;player.tiley=y;ConnectAreas();
            if (TryMove(&player) && CheckLine(ob)) { found=true;Aim(ob); }
        }
    CHECK(found);CHECK(ThreeDRefresh() && DrawStatus());
}
static void Capture(const char *name)
{
    const char *dir=getenv("WOLF3D_CAPTURE_DIR");if (!dir) return;
    char path[1024];int n=snprintf(path,sizeof(path),"%s/e%u-%s.bmp",dir,ep+1,name);
    CHECK(n>0 && (size_t)n<sizeof(path));CHECK(VW_UpdateScreen() && VL_Capture(path));
}
static void CollectKeyAndExit(void)
{
    bool collected=false;
    const int dx[]={1,0,-1,0},dy[]={0,-1,0,1};
    for (unsigned i=0;i<statcount && !collected;++i) if (statobjlist[i].itemnumber==bo_key1 && statobjlist[i].shapenum>=0) {
        for (unsigned d=0;d<4 && !collected;++d) {
            player.x=statobjlist[i].tilex*TILEGLOBAL+32768-dx[d]*0xa000;
            player.y=statobjlist[i].tiley*TILEGLOBAL+32768-dy[d]*0xa000;
            player.tilex=player.x/TILEGLOBAL;player.tiley=player.y/TILEGLOBAL;player.angle=(int)d*90;
            if (!TryMove(&player)) continue;
            CHECK(ThreeDRefresh());collected=(gamestate.keys&1)!=0;
        }
    }
    CHECK(collected);
    bool exited=false;
    for (unsigned i=0;i<4096 && !exited;++i) if (CA_Map()->planes[1][i]==99) {
        player.tilex=(int)(i%64);player.tiley=(int)(i/64);
        player.x=player.tilex*TILEGLOBAL+32768;player.y=player.tiley*TILEGLOBAL+32768;
        player.angle=90;if (!TryMove(&player)) continue;
        Thrust(90,0);exited=gamestate.victoryflag;
    }
    CHECK(exited);
}
static void Episode(bool owned)
{
    CHECK(DemoLoopStartGame(ticks));Steps(11);
    CHECK(GameStartSelected(ticks,ep,2));ShutdownGameLevel();gamestate.mapon=8;CHECK(SetupGameLevel());
    const int bosses[]={bossobj,schabbobj,mechahitlerobj,giftobj,gretelobj,fatobj};
    objtype *boss=Find(bosses[ep]);CHECK(boss);Arrange(boss);Capture("boss-start");
    GiveWeapon(wp_chaingun);gamestate.ammo=99;int initial=boss->hitpoints;Key(SDL_SCANCODE_LCTRL,true);
    unsigned elapsed=0;bool transformed=false;
    while (elapsed++<12000) {
        if (!(boss->flags&FL_SHOOTABLE)) {
            if (ep!=2 || transformed) break;
            objtype *hitler=Find(realhitlerobj);
            if (hitler) { boss=hitler;transformed=true;Arrange(boss);Capture("hitler-morph"); }
        }
        gamestate.health=100;gamestate.ammo=99;
        if (boss->flags&FL_SHOOTABLE) {
            if (!CheckLine(boss) || (elapsed%70)==0) Arrange(boss);
            Aim(boss);CHECK(ThreeDRefresh());
        }
        Step();CHECK(!gamestate.died);
    }
    Key(SDL_SCANCODE_LCTRL,false);Step();
    CHECK(elapsed<12000 && boss->hitpoints<=0 && initial>0);
    CHECK(ep!=2 || transformed);Capture("boss-dead");
    if (ep==0 || ep==4) CollectKeyAndExit();
    unsigned waits=0;bool camera=false,replay=false;
    while (!GameVictoryPresentation() && waits++<2500) {
        gamestate.health=100;Step();
        if (DeathCameraActive() && !camera) { Capture("death-camera");camera=true; }
        if (gamestate.victoryflag && !DeathCameraActive() && !replay) { Capture("victory-replay");replay=true; }
    }
    CHECK(GameVictoryPresentation() && DemoLoopState()==UI_GAME);Capture("statistics");
    unsigned map=(unsigned)(ep*10+8);CHECK(CA_Map()->number==map);
    Tap(SDL_SCANCODE_RETURN);Capture("ending-page1");
    Tap(SDL_SCANCODE_RIGHT);Capture("ending-page2");
    Tap(SDL_SCANCODE_LEFT);Tap(SDL_SCANCODE_ESCAPE);Steps(21);
    CHECK(DemoLoopState()==UI_SCORES && !CA_Map() && !player.next);
    Tap(SDL_SCANCODE_RETURN);Steps(21);CHECK(DemoLoopState()==UI_MENU);
    printf("E%u controlled %s weapon fight, finale, two article pages and menu return passed.\n",ep+1,owned ? "owned" : "synthetic");
}
int main(int argc,char **argv)
{
    CHECK(argc==2 || argc==3);MM_Startup();CHECK(CA_Startup(argv[1]));CHECK(VL_Startup());
    CHECK(DemoLoopStartup(0,true));Steps(11);
    if (argc==3) for (ep=0;ep<6;++ep) Episode(true);
    else { ep=1;Episode(false); }
    GameStop();VL_Shutdown();CA_Shutdown();MM_Shutdown();CHECK(!MM_TotalBytes());return 0;
}
