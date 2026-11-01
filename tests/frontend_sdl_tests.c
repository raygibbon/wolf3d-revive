/* SPDX-License-Identifier: GPL-2.0-only */
#include "engine/WL_MAIN.h"
#include "engine/WL_MENU.h"
#include "engine/ID_CA.h"
#include "engine/ID_IN.h"
#include "platform/sdl_video.h"
#include <SDL3/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(x) do { if (!(x)) { fprintf(stderr,"%d: %s (%s; %s)\n",__LINE__,#x,CA_Error(),VL_Error()); exit(1); } } while (0)
static void Key(SDL_Scancode scan, bool down)
{
    SDL_Event event; memset(&event,0,sizeof(event));
    event.type = down ? SDL_EVENT_KEY_DOWN : SDL_EVENT_KEY_UP;
    event.key.scancode = scan; CHECK(SDL_PushEvent(&event));
}
static void Tap(SDL_Scancode scan) { Key(scan,true); Key(scan,false); }
static void Step(uint64_t ticks) { CHECK(!VL_PollQuit()); CHECK(DemoLoop(ticks)); CHECK(VW_UpdateScreen()); }
int main(int argc,char **argv)
{
    CHECK(argc == 2); MM_Startup(); CHECK(CA_Startup(argv[1])); CHECK(VL_Startup());
    CHECK(DemoLoopStartup(0,false));
    Tap(SDL_SCANCODE_RETURN); Step(0); Step(40); CHECK(DemoLoopState() == UI_MENU);
    Key(SDL_SCANCODE_DOWN,true); Step(41); CHECK(MainItems.curpos == backtodemo);
    Step(60); CHECK(MainItems.curpos == backtodemo);
    Step(61); CHECK(MainItems.curpos == quit);
    Key(SDL_SCANCODE_DOWN,false); Step(62);
    Tap(SDL_SCANCODE_RETURN); Step(63); CHECK(DemoLoopState() == UI_CONFIRM);
    Tap(SDL_SCANCODE_N); Step(64); CHECK(DemoLoopState() == UI_MENU);
    Tap(SDL_SCANCODE_V); Step(65); CHECK(MainItems.curpos == viewscores);
    Tap(SDL_SCANCODE_RETURN); Step(66); Step(86); CHECK(DemoLoopState() == UI_SCORES);
    Tap(SDL_SCANCODE_ESCAPE); Step(87); Step(107); CHECK(DemoLoopState() == UI_MENU);
    Tap(SDL_SCANCODE_Q); Tap(SDL_SCANCODE_RETURN); Step(108); CHECK(DemoLoopState() == UI_CONFIRM);
    Tap(SDL_SCANCODE_Y); Step(109); Step(119); CHECK(DemoLoopState() == UI_DONE);
    VL_Shutdown(); CA_Shutdown(); MM_Shutdown(); CHECK(MM_TotalBytes() == 0);
    puts("SDL events drive original menu, scores, held navigation and clean quit.");
    return 0;
}
