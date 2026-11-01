/* SPDX-License-Identifier: GPL-2.0-only */
#include "engine/WL_DEF.h"
#include "engine/WL_MAIN.h"
#include "engine/WL_MENU.h"
#include "engine/ID_CA.h"
#include "platform/sdl_video.h"
#include <SDL3/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(x) do { if (!(x)) { fprintf(stderr,"%d: %s (%s; %s)\n",__LINE__,#x,CA_Error(),VL_Error());exit(1); } } while (0)
static uint64_t ticks;
static void Step(unsigned delta) { ticks+=delta;CHECK(!VL_PollQuit());CHECK(DemoLoop(ticks));CHECK(VW_UpdateScreen()); }
static void Tap(SDL_Scancode scan)
{
    SDL_Event e;memset(&e,0,sizeof(e));e.key.scancode=scan;
    e.type=SDL_EVENT_KEY_DOWN;CHECK(SDL_PushEvent(&e));
    e.type=SDL_EVENT_KEY_UP;CHECK(SDL_PushEvent(&e));Step(1);
}
static void Capture(int argc,char **argv,const char *name)
{
    if (argc<3 || strcmp(argv[2],"--missing-next")==0) return;
    char path[1024];int n=snprintf(path,sizeof(path),"%s/%s.bmp",argv[2],name);
    CHECK(n>0 && (size_t)n<sizeof(path));CHECK(VL_Capture(path));
}
int main(int argc,char **argv)
{
    CHECK(argc==2 || argc==3);MM_Startup();CHECK(CA_Startup(argv[1]));CHECK(VL_Startup());
    CHECK(GameProbe());MainMenu[newgame].active=1;MainItems.curpos=newgame;
    CHECK(DemoLoopStartup(0,true));Step(21);
    Tap(SDL_SCANCODE_RETURN);Step(21);CHECK(DemoLoopState()==UI_EPISODE);
    Capture(argc,argv,"episodes");
    Tap(SDL_SCANCODE_RETURN);Step(21);CHECK(DemoLoopState()==UI_DIFFICULTY);
    Capture(argc,argv,"difficulty");
    Tap(SDL_SCANCODE_ESCAPE);Step(21);CHECK(DemoLoopState()==UI_EPISODE);
    Tap(SDL_SCANCODE_DOWN);Tap(SDL_SCANCODE_RETURN);Step(21);CHECK(DemoLoopState()==UI_DIFFICULTY);
    Tap(SDL_SCANCODE_DOWN);Tap(SDL_SCANCODE_RETURN);Step(21);CHECK(DemoLoopState()==UI_GAME);
    CHECK(gamestate.episode==1 && gamestate.difficulty==3 && CA_Map()->number==10);
    Capture(argc,argv,"episode2-hard");
    Tap(SDL_SCANCODE_ESCAPE);Step(21);CHECK(DemoLoopState()==UI_MENU);
    /* Start the default E1M1 independently of the last selected menu choice. */
    CHECK(DemoLoopStartGame(ticks));Step(11);CHECK(gamestate.episode==0 && gamestate.difficulty==2);
    bool pushed=false;
    static const int dx[]={1,0,-1,0},dy[]={0,-1,0,1};
    for (int y=2;y<62 && !pushed;++y) for (int x=2;x<62 && !pushed;++x) {
        if (!pushable[x][y]) continue;
        for (unsigned d=0;d<4;++d) {
            int px0=x-dx[d],py0=y-dy[d];
            if (tilemap[px0][py0] || tilemap[x+dx[d]][y+dy[d]] ||
                staticblock[x+dx[d]][y+dy[d]] || actorat[x+dx[d]][y+dy[d]]) continue;
            player.tilex=px0;player.tiley=py0;player.x=px0*TILEGLOBAL+32768;
            player.y=py0*TILEGLOBAL+32768;player.angle=(int)d*90;
            if (!TryMove(&player)) continue;
            CHECK(ThreeDRefresh() && VW_UpdateScreen());Capture(argc,argv,"pushwall-before");
            Tap(SDL_SCANCODE_SPACE);CHECK(pwallstate==1 && gamestate.secretcount==1);
            for (unsigned t=0;t<63;++t) Step(1);
            CHECK(pwallstate==64 && pwallpos==32);Capture(argc,argv,"pushwall-moving");
            for (unsigned t=0;t<192;++t) Step(1);
            CHECK(!pwallstate && !pushable[x][y]);pushed=true;break;
        }
    }
    CHECK(pushed);Capture(argc,argv,"pushwall-open");
    CHECK(DemoLoopStartGame(ticks));Step(11);
    /* Synthetic campaign has the normal switch at 4,2. For owned captures,
       locate a real elevator and use its actual neighbouring floor instead. */
    int ex=-1,ey=-1,px=0;
    for (int y=1;y<63 && ex<0;++y) for (int x=1;x<63;++x) if (tilemap[x][y]==21) {
        if (!tilemap[x-1][y]) { ex=x;ey=y;px=x-1;break; }
        if (!tilemap[x+1][y]) { ex=x;ey=y;px=x+1;break; }
    }
    CHECK(ex>=0);player.tilex=px;player.tiley=ey;player.x=px*TILEGLOBAL+32768;
    player.y=ey*TILEGLOBAL+32768;player.angle=px<ex ? 0 : 180;
    Tap(SDL_SCANCODE_SPACE);CHECK(GameIntermission());Capture(argc,argv,"intermission");
    int score=gamestate.score;Tap(SDL_SCANCODE_RETURN);CHECK(!GameIntermission());
    if (argc==3 && strcmp(argv[2],"--missing-next")==0) {
        CHECK(DemoLoopState()==UI_ERROR && !CA_Map());
        Tap(SDL_SCANCODE_RETURN);Step(21);CHECK(DemoLoopState()==UI_MENU);
        GameStop();VL_Shutdown();CA_Shutdown();MM_Shutdown();CHECK(!MM_TotalBytes());
        puts("Failed next-floor load reports an error and returns to menu.");return 0;
    }
    CHECK(CA_Map()->number==1 && gamestate.oldscore==score);
    Capture(argc,argv,"floor2");
    GameStop();VL_Shutdown();CA_Shutdown();MM_Shutdown();CHECK(!MM_TotalBytes());
    puts("SDL episode/difficulty selection, cancellation, elevator, intermission and next floor passed.");return 0;
}
