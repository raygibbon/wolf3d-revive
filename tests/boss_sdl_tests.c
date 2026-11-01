/* SPDX-License-Identifier: GPL-2.0-only */
/* Controlled encounter checks, not level or episode playthroughs. */
#include "engine/WL_DEF.h"
#include "engine/ID_CA.h"
#include "engine/ID_IN.h"
#include "engine/ID_VL.h"
#include "platform/sdl_video.h"
#include <SDL3/SDL.h>
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#define CHECK(x) do { if (!(x)) { fprintf(stderr,"%d: %s (%s; %s)\n",__LINE__,#x,CA_Error(),VL_Error());exit(1); } } while (0)
static uint64_t ticks;
static unsigned shots;
static void Sound(soundnames sound,const objtype *ob)
{
    (void)ob;if (sound==NAZIFIRESND || sound==BOSSFIRESND) ++shots;
}
static void Step(void)
{
    CHECK(!VL_PollQuit());CHECK(GameStep(++ticks));CHECK(VW_UpdateScreen());
}
static void Capture(const char *name)
{
    const char *dir=getenv("WOLF3D_CAPTURE_DIR");if (!dir) return;
    char path[1024];int n=snprintf(path,sizeof(path),"%s/%s.bmp",dir,name);
    CHECK(n>0 && (size_t)n<sizeof(path));CHECK(VL_Capture(path));
}
static objtype *Find(int kind)
{
    for (unsigned i=0;i<objcount;++i) if (objlist[i].obclass==kind) return &objlist[i];
    CHECK(false);return NULL;
}
static void FaceActor(objtype *ob)
{
    /* Locate a nearby empty tile with a clear line, without altering the map. */
    bool placed=false;
    for (int distance=2;distance<10 && !placed;++distance)
        for (int dy=-distance;dy<=distance && !placed;++dy)
            for (int dx=-distance;dx<=distance && !placed;++dx) {
                if (abs(dx)!=distance && abs(dy)!=distance) continue;
                int x=ob->tilex+dx,y=ob->tiley+dy;
                if (x<1 || y<1 || x>62 || y>62) continue;
                player.x=x*TILEGLOBAL+32768;player.y=y*TILEGLOBAL+32768;
                player.tilex=x;player.tiley=y;
                double angle=atan2((double)player.y-ob->y,(double)ob->x-player.x);
                if (angle<0) angle+=6.28318530717958647692;
                player.angle=(int)(angle*360/6.28318530717958647692);
                ConnectAreas();
                if (TryMove(&player) && CheckLine(ob)) placed=true;
            }
    CHECK(placed);CHECK(ThreeDRefresh() && DrawStatus() && VW_UpdateScreen());
}
int main(int argc,char **argv)
{
    CHECK(argc==2 || argc==3);MM_Startup();CHECK(CA_Startup(argv[1]));CHECK(VL_Startup());
    CHECK(VL_SetPalette(gamepal,768));SetGameSoundHook(Sound);
    bool owned=argc==3;
    CHECK(GameStartSelected(0,owned ? 1U : 0U,2));
    if (owned) { ShutdownGameLevel();gamestate.mapon=7;CHECK(SetupGameLevel()); }
    objtype *boss=Find(bossobj);
    /* E2M8 Hans encounters start in enclosed rooms. Open nearby real doors
       through the existing door API before arranging this controlled shot. */
    if (owned) {
        for (unsigned i=0;i<doornum;++i)
            if (abs((int)doorobjlist[i].x-boss->tilex)<10 &&
                abs((int)doorobjlist[i].y-boss->tiley)<10) OpenDoor(i);
        for (unsigned tic=0;tic<70;++tic) MoveDoors();
    }
    FaceActor(boss);Capture("boss-standing");
    /* Deliver a real SDL key edge to the existing weapon simulation. */
    int hp=boss->hitpoints;SDL_Event event={0};
    event.type=SDL_EVENT_KEY_DOWN;event.key.scancode=SDL_SCANCODE_LCTRL;CHECK(SDL_PushEvent(&event));
    Step();
    event.type=SDL_EVENT_KEY_UP;CHECK(SDL_PushEvent(&event));
    for (unsigned i=0;i<55;++i) { gamestate.health=100;Step(); }
    CHECK(boss->hitpoints<hp && gamestate.ammo==7 && !Keyboard[0x1d]);
    for (unsigned i=0;i<250 && !shots;++i) { gamestate.health=100;Step(); }
    CHECK(shots);Capture("boss-combat");
    /* Ghost-only owned secret map (E3M10); do not erase any encounters. */
    if (owned) { ShutdownGameLevel();gamestate.episode=2;gamestate.mapon=9;CHECK(SetupGameLevel()); }
    objtype *ghost=Find(ghostobj);FaceActor(ghost);Capture("ghost-standing");
    int32_t x=ghost->x,y=ghost->y;
    for (unsigned i=0;i<70;++i) { gamestate.health=100;Step(); }
    CHECK(ghost->x!=x || ghost->y!=y);Capture("ghost-moving");
    SetGameSoundHook(NULL);GameStop();VL_Shutdown();CA_Shutdown();MM_Shutdown();CHECK(!MM_TotalBytes());
    puts("SDL input damages a boss; boss fires; ghosts render and chase with owned or synthetic data.");
    return 0;
}
