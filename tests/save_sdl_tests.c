/* SPDX-License-Identifier: GPL-2.0-only */
#include "engine/WL_SAVE.h"
#include "engine/WL_MAIN.h"
#include "engine/WL_MENU.h"
#include "engine/ID_CA.h"
#include "engine/ID_SD.h"
#include "platform/sdl_video.h"
#include "platform/sdl_audio.h"
#include "platform/save_file.h"
#include <SDL3/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(x) do { if(!(x)) { fprintf(stderr,"%d: %s (%s; %s)\n",__LINE__,#x,CA_Error(),Audio_Error());exit(1); } } while(0)
static uint64_t ticks;
static void Step(void) { CHECK(!VL_PollQuit());CHECK(DemoLoop(++ticks));Audio_Pump();CHECK(VW_UpdateScreen()); }
static void Steps(unsigned n) { while(n--) Step(); }
static void Tap(SDL_Scancode code)
{
    SDL_Event e={0};e.key.scancode=code;e.type=SDL_EVENT_KEY_DOWN;CHECK(SDL_PushEvent(&e));
    e.type=SDL_EVENT_KEY_UP;CHECK(SDL_PushEvent(&e));Step();
}
static void Menu(int choice) { MainItems.curpos=choice;Tap(SDL_SCANCODE_RETURN);Steps(21); }
static void Capture(const char *name)
{
    const char *dir=getenv("WOLF3D_CAPTURE_DIR");if(!dir) return;
    char path[4096];(void)snprintf(path,sizeof(path),"%s/%s.bmp",dir,name);CHECK(VL_Capture(path));
}
int main(int argc,char **argv)
{
    CHECK(argc==4);MM_Startup();CHECK(CA_Startup(argv[1]));CHECK(VL_Startup());
    SD_SetDigitizedMode(true);CHECK(Audio_Startup());
    char savepath[4096];(void)snprintf(savepath,sizeof(savepath),"%s%s",argv[2],getenv("WOLF3D_TEST_UNICODE")?"/\xc3\xa9":"");
    CHECK(SDL_CreateDirectory(savepath));CHECK(Save_SetDirectory(savepath));
    CHECK(GameProbe());MainMenu[newgame].active=1;MainItems.curpos=newgame;
    CHECK(DemoLoopStartup(0,true));Steps(11);
    if(!strcmp(argv[3],"read")) {
        Menu(loadgame);CHECK(DemoLoopState()==UI_LOAD);Tap(SDL_SCANCODE_RETURN);Steps(21);
        CHECK(DemoLoopState()==UI_GAME && GameRunning() && gamestate.score==8842 && gamestate.health==76 && gamestate.ammo==62);
        sd_status audio;SD_GetStatus(&audio);CHECK(audio.music==SD_LevelMusic(0,0));
    } else {
        CHECK(DemoLoopStartGame(ticks));Steps(11);
        gamestate.score=8842;gamestate.health=76;gamestate.ammo=62;
        Tap(SDL_SCANCODE_ESCAPE);Steps(21);CHECK(DemoLoopState()==UI_MENU && GameRunning() && CA_Map());
        uint32_t clock=gamestate.TimeCount;Steps(70);CHECK(gamestate.TimeCount==clock);
        Menu(savegame);CHECK(DemoLoopState()==UI_SAVE);Capture("save-slots");
        Tap(SDL_SCANCODE_RETURN);Tap(SDL_SCANCODE_A);Tap(SDL_SCANCODE_1);Capture("save-description");
        Tap(SDL_SCANCODE_RETURN);Steps(21);CHECK(DemoLoopState()==UI_MENU);
        save_info info;Save_ReadInfo(0,&info);CHECK(info.status==SAVE_VALID && !strcmp(info.description,"a1"));
        Menu(savegame);Tap(SDL_SCANCODE_RETURN);Capture("save-overwrite");Tap(SDL_SCANCODE_N);
        Tap(SDL_SCANCODE_RETURN);Tap(SDL_SCANCODE_Y);Tap(SDL_SCANCODE_BACKSPACE);Tap(SDL_SCANCODE_B);Tap(SDL_SCANCODE_ESCAPE);
        Save_ReadInfo(0,&info);CHECK(!strcmp(info.description,"a1"));
        Tap(SDL_SCANCODE_RETURN);Tap(SDL_SCANCODE_Y);Tap(SDL_SCANCODE_B);Tap(SDL_SCANCODE_RETURN);Steps(21);
        Save_ReadInfo(0,&info);CHECK(!strcmp(info.description,"a1b"));
        gamestate.score=12345;Menu(loadgame);CHECK(DemoLoopState()==UI_LOAD);Capture("load-slots");
        Tap(SDL_SCANCODE_DOWN);Tap(SDL_SCANCODE_RETURN);CHECK(DemoLoopState()==UI_LOAD && gamestate.score==12345);
        Tap(SDL_SCANCODE_ESCAPE);Tap(SDL_SCANCODE_UP);Tap(SDL_SCANCODE_RETURN);Steps(21);
        CHECK(DemoLoopState()==UI_GAME && gamestate.score==8842 && gamestate.health==76 && gamestate.ammo==62);
        Tap(SDL_SCANCODE_ESCAPE);Steps(21);Menu(loadgame);Tap(SDL_SCANCODE_ESCAPE);Steps(21);
        CHECK(DemoLoopState()==UI_MENU && GameRunning());
        char path[4128];uint8_t bad[96]={0};
        CHECK(strlen(savepath)<sizeof(path)-20);
        (void)snprintf(path,sizeof(path),"%s/slot2.w3s",savepath);
        CHECK(SaveFile_WriteAtomic(path,bad,sizeof(bad),true));
        memcpy(bad,"W3RSAVE",8);bad[8]=99;
        (void)snprintf(path,sizeof(path),"%s/slot3.w3s",savepath);
        CHECK(SaveFile_WriteAtomic(path,bad,sizeof(bad),true));
        Save_ReadInfo(1,&info);CHECK(info.status==SAVE_CORRUPT);
        Save_ReadInfo(2,&info);CHECK(info.status==SAVE_UNSUPPORTED);
        Menu(loadgame);Capture("load-invalid-slots");Tap(SDL_SCANCODE_DOWN);Tap(SDL_SCANCODE_RETURN);
        CHECK(DemoLoopState()==UI_LOAD && gamestate.score==8842);Tap(SDL_SCANCODE_ESCAPE);
        Tap(SDL_SCANCODE_ESCAPE);Steps(21);Menu(savegame);Tap(SDL_SCANCODE_RETURN);Tap(SDL_SCANCODE_N);
        Save_ReadInfo(1,&info);CHECK(info.status==SAVE_CORRUPT);
        Tap(SDL_SCANCODE_RETURN);Tap(SDL_SCANCODE_Y);Tap(SDL_SCANCODE_B);Tap(SDL_SCANCODE_RETURN);Steps(21);
        Save_ReadInfo(1,&info);CHECK(info.status==SAVE_VALID && !strcmp(info.description,"b"));
        Menu(backtodemo);CHECK(DemoLoopState()==UI_GAME);
    }
    GameStop();Audio_Shutdown();VL_Shutdown();CA_Shutdown();MM_Shutdown();CHECK(!MM_TotalBytes());
    puts("SDL save/load, descriptions, overwrite/cancel, pause/resume and restart passed.");return 0;
}
