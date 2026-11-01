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
    Tap(SDL_SCANCODE_N);Tap(SDL_SCANCODE_RETURN);Step(11);Step(31);CHECK(DemoLoopState()==UI_EPISODE);
    Tap(SDL_SCANCODE_RETURN);Step(32);Step(52);CHECK(DemoLoopState()==UI_DIFFICULTY);
    Tap(SDL_SCANCODE_RETURN);Step(53);Step(73);CHECK(DemoLoopState()==UI_GAME);
    int32_t x=player.x;Key(SDL_SCANCODE_UP,true);Step(74);CHECK(player.x>x);Step(82);x=player.x;
    SDL_Event event;memset(&event,0,sizeof(event));event.type=SDL_EVENT_WINDOW_FOCUS_LOST;CHECK(SDL_PushEvent(&event));
    Step(83);CHECK(player.x==x);
    Tap(SDL_SCANCODE_RIGHT);Step(84);int angle=player.angle;CHECK(angle==359);Step(85);CHECK(player.angle==angle);
    Key(SDL_SCANCODE_LCTRL,true);Step(86);CHECK(gamestate.attackframe==1);
    Key(SDL_SCANCODE_RCTRL,true);Step(92);
    Key(SDL_SCANCODE_LCTRL,false);Step(98);CHECK(gamestate.ammo==7);
    Step(104);Step(110);Step(112);CHECK(gamestate.ammo==7 && !gamestate.attackframe);
    event.type=SDL_EVENT_WINDOW_FOCUS_LOST;CHECK(SDL_PushEvent(&event));Step(113);
    Tap(SDL_SCANCODE_RCTRL);Step(114);Step(120);Step(126);CHECK(gamestate.ammo==6);
    Tap(SDL_SCANCODE_ESCAPE);Tap(SDL_SCANCODE_RETURN);Step(127);Step(147);CHECK(DemoLoopState()==UI_MENU);
    Step(148);CHECK(DemoLoopState()==UI_MENU); /* No Enter leakage into a second game. */
    GameStop();VL_Shutdown();CA_Shutdown();MM_Shutdown();CHECK(!MM_TotalBytes());
    puts("SDL New Game, movement, Ctrl aliases, semi-auto fire, focus loss and menu transitions passed.");return 0;
}
