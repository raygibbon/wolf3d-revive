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
#define CHECK(x) do { if (!(x)) { fprintf(stderr,"%d: %s (%s; %s)\n",__LINE__,#x,CA_Error(),VL_Error()); exit(1); } } while(0)
static void Key(SDL_Scancode scan,bool down)
{
    SDL_Event event;memset(&event,0,sizeof(event));event.type=down ? SDL_EVENT_KEY_DOWN : SDL_EVENT_KEY_UP;
    event.key.scancode=scan;CHECK(SDL_PushEvent(&event));
}
static void Tap(SDL_Scancode scan) { Key(scan,true);Key(scan,false); }
static void Step(uint64_t ticks) { CHECK(!VL_PollQuit());CHECK(DemoLoop(ticks));CHECK(VW_UpdateScreen()); }
int main(int argc,char **argv)
{
    CHECK(argc==2);MM_Startup();CHECK(CA_Startup(argv[1]));CHECK(VL_Startup());CHECK(GameProbe());
    MainMenu[newgame].active=1;MainItems.curpos=viewscores;CHECK(DemoLoopStartup(0,true));Step(10);
    Tap(SDL_SCANCODE_N);Tap(SDL_SCANCODE_RETURN);Step(11);Step(31);CHECK(DemoLoopState()==UI_GAME);
    int32_t x=player.x;Key(SDL_SCANCODE_UP,true);Step(32);CHECK(player.x>x);Step(40);x=player.x;
    SDL_Event event;memset(&event,0,sizeof(event));event.type=SDL_EVENT_WINDOW_FOCUS_LOST;CHECK(SDL_PushEvent(&event));
    Step(41);CHECK(player.x==x);
    Tap(SDL_SCANCODE_RIGHT);Step(42);int angle=player.angle;CHECK(angle==359);Step(43);CHECK(player.angle==angle);
    Tap(SDL_SCANCODE_ESCAPE);Tap(SDL_SCANCODE_RETURN);Step(44);Step(64);CHECK(DemoLoopState()==UI_MENU);
    Step(65);CHECK(DemoLoopState()==UI_MENU); /* No Enter leakage into a second game. */
    GameStop();VL_Shutdown();CA_Shutdown();MM_Shutdown();CHECK(!MM_TotalBytes());
    puts("SDL New Game, movement, quick taps, focus loss and Escape/menu transitions passed.");return 0;
}
