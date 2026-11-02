/* SPDX-License-Identifier: GPL-2.0-only */
#include "engine/WL_CONTROLS.h"
#include "engine/WL_SAVE.h"
#include "engine/WL_MAIN.h"
#include "engine/WL_MENU.h"
#include "engine/ID_CA.h"
#include "engine/ID_IN.h"
#include "engine/ID_US_1.h"
#include "engine/ID_VL.h"
#include "platform/sdl_video.h"
#include "platform/sdl_input.h"
#include <SDL3/SDL.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(x) do { if(!(x)) { fprintf(stderr,"%d: %s (%s; %s)\n",__LINE__,#x,CA_Error(),SDL_GetError());exit(1); } } while(0)
static uint64_t ticks;
static void Arena(void)
{
    InitObjects();doornum=0;ResetPushwalls();
    for(int x=0;x<64;++x) for(int y=0;y<64;++y) { tilemap[x][y]=(uint8_t)((!x || !y || x==63 || y==63)?1:0);areaMap[x][y]=0; }
    player.x=player.y=10*TILEGLOBAL+TILEGLOBAL/2;player.tilex=player.tiley=10;player.angle=0;
    player.areanumber=0;player.flags=0;ResetMovement();IN_ClearKeysDown();
}
static void One(void) { CHECK(GameStep(++ticks)); }
static void Reset(ct_preset preset) { CT_Reset(preset);CHECK(GameStart(ticks));Arena(); }
static double Distance(int32_t x,int32_t y) { return hypot((double)player.x-x,(double)player.y-y); }
static void Core(void)
{
    /* Equivalent forward inputs must leave the entire simulation identical,
       including original enemies and gameplay RNG, in both presets. */
    CT_Reset(CT_CLASSIC);CHECK(GameStart(0));ticks=0;CT_Key(SDL_SCANCODE_UP,true);
    for(unsigned i=0;i<20;++i) One();
    CHECK(Save_Write(4,"classic equivalent",true));unsigned equivalent_rng=US_RndIndex();
    CT_Reset(CT_MODERN);CHECK(GameStart(0));ticks=0;CT_Key(SDL_SCANCODE_W,true);
    for(unsigned i=0;i<20;++i) One();
    CHECK(Save_Write(5,"modern equivalent",true) && US_RndIndex()==equivalent_rng);
    Reset(CT_CLASSIC);int32_t x=player.x,y=player.y;
    CT_Key(SDL_SCANCODE_W,true);One();CHECK(player.x==x && player.y==y);CT_Clear();
    CT_Key(SDL_SCANCODE_UP,true);One();CHECK(player.x==x+5249 && player.y==y);
    Arena();CT_Key(SDL_SCANCODE_UP,true);CT_Key(SDL_SCANCODE_LSHIFT,true);One();CHECK(player.x==x+10499);
    Arena();CT_Key(SDL_SCANCODE_RIGHT,true);One();CHECK(player.angle==359 && player.x==x);
    Arena();CT_Key(SDL_SCANCODE_RIGHT,true);CT_Key(SDL_SCANCODE_LALT,true);One();CHECK(player.angle==0 && player.y==y+5249);
    Arena();CT_Key(SDL_SCANCODE_UP,true);CT_Key(SDL_SCANCODE_DOWN,true);One();CHECK(player.x==x && player.y==y);
    Arena();CT_Key(SDL_SCANCODE_KP_8,true);One();CHECK(player.x==x+5249);
    Arena();CHECK(CT_Bind(CT_STRAFE_LEFT,0,SDL_SCANCODE_A));CT_Key(SDL_SCANCODE_A,true);One();CHECK(player.y==y-5249);
    Reset(CT_MODERN);CT_Key(SDL_SCANCODE_W,true);One();double axial=Distance(x,y);CHECK(axial==5249);
    Arena();CT_Key(SDL_SCANCODE_S,true);One();CHECK(player.x==x-3499);
    Arena();CT_Key(SDL_SCANCODE_A,true);One();CHECK(player.y==y-5249 && player.angle==0);
    Arena();CT_Key(SDL_SCANCODE_D,true);One();CHECK(player.y==y+5249 && player.angle==0);
    Arena();CT_Key(SDL_SCANCODE_W,true);CT_Key(SDL_SCANCODE_UP,true);One();CHECK(Distance(x,y)==axial);
    Arena();CT_Key(SDL_SCANCODE_W,true);CT_Key(SDL_SCANCODE_D,true);One();CHECK(Distance(x,y)<=axial && Distance(x,y)>axial-4 && thrustspeed==5250);
    Arena();CT_Key(SDL_SCANCODE_W,true);CT_Key(SDL_SCANCODE_D,true);CT_Key(SDL_SCANCODE_LSHIFT,true);One();CHECK(Distance(x,y)<=10499 && Distance(x,y)>10495 && thrustspeed==10500);
    Arena();CT_Key(SDL_SCANCODE_W,true);CT_Key(SDL_SCANCODE_S,true);CT_Key(SDL_SCANCODE_A,true);CT_Key(SDL_SCANCODE_D,true);One();CHECK(player.x==x && player.y==y);
    Arena();CT_Key(SDL_SCANCODE_D,true);CT_Key(SDL_SCANCODE_LALT,true);CT_Key(SDL_SCANCODE_RIGHT,true);One();CHECK(player.y==y+5249);
    Arena();CT_Key(SDL_SCANCODE_A,true);CT_Key(SDL_SCANCODE_LALT,true);CT_Key(SDL_SCANCODE_RIGHT,true);One();CHECK(player.x==x && player.y==y);
    /* One clipped vector and one tile update; never move into a static block. */
    Arena();staticblock[11][10]=true;CT_Key(SDL_SCANCODE_W,true);for(unsigned i=0;i<12;++i) One();CHECK(player.tilex==10);
    Arena();unsigned rng=US_RndIndex();CT_MouseMotion(2);CT_MouseMotion(2);CHECK(US_RndIndex()==rng);One();CHECK(player.angle==358);
    Arena();CT_SetSensitivity(20);CT_MouseMotion(4);One();CHECK(player.angle==356);
    Arena();CT_SetInvert(true);CT_MouseMotion(4);One();CHECK(player.angle==4);
    Arena();CT_SetInvert(false);CT_MouseMotion(1e30);CT_Key(SDL_SCANCODE_RIGHT,true);One();CHECK(player.angle>=0 && player.angle<360);
    Arena();CT_SetSensitivity(1);CT_MouseMotion(0.5);CHECK(CT_MouseTurn()==0);CT_MouseMotion(0.5);CHECK(CT_MouseTurn()==1);
    CT_SetMouse(false);CT_MouseMotion(100);CT_MouseButton(1,true);CHECK(CT_MouseTurn()==0 && !CT_Down(CT_FIRE));
    CT_Reset(CT_MODERN);CT_MouseMotion(NAN);CT_MouseMotion(INFINITY);CT_MouseMotion(-INFINITY);CHECK(CT_MouseTurn()==0);CT_MouseButton(1,true);CT_MouseButton(1,false);CHECK(CT_Down(CT_FIRE));CT_EndTic();CHECK(!CT_Down(CT_FIRE));
    CT_MouseButton(3,true);CHECK(CT_Pressed(CT_USE) && !CT_Pressed(CT_USE));CT_Clear();
    CHECK(!CT_Bind(CT_FORWARD,0,SDL_SCANCODE_S) && CT_Binding(CT_FORWARD,0)==SDL_SCANCODE_W);
    CHECK(!CT_Bind(CT_FIRE,0,SDL_SCANCODE_ESCAPE));CHECK(!CT_Bind(CT_FIRE,2,4));CHECK(!CT_Bind(CT_FIRE,0,512));
    CHECK(CT_Bind(CT_FORWARD,0,SDL_SCANCODE_E));CT_Key(SDL_SCANCODE_W,true);CHECK(!CT_Down(CT_FORWARD));CT_Key(SDL_SCANCODE_E,true);CHECK(CT_Down(CT_FORWARD));
    CHECK(CT_Bind(CT_FORWARD,1,0) && CT_Bind(CT_FIRE,0,SDL_SCANCODE_UP));
    IN_KeyEvent(sc_UpArrow,true);CHECK(IN_ConsumePress(sc_UpArrow) && CT_Down(CT_FIRE));IN_ClearKeysDown();
    CHECK(CT_Bind(CT_WEAPON2,0,SDL_SCANCODE_R));gamestate.bestweapon=wp_chaingun;gamestate.weapon=wp_knife;
    CT_Key(SDL_SCANCODE_2,true);CheckWeaponChange();CHECK(gamestate.weapon==wp_knife);CT_Clear();
    CT_Key(SDL_SCANCODE_R,true);CheckWeaponChange();CHECK(gamestate.weapon==wp_pistol);
    CT_Reset(CT_CLASSIC);CHECK(CT_Binding(CT_FORWARD,0)==SDL_SCANCODE_UP && !CT_MouseEnabled());
    /* Controls/settings are not save-format fields. Restore angle fraction,
       clear pending input, and leave the player's current settings intact. */
    Reset(CT_MODERN);CT_SetSensitivity(17);CT_MouseMotion(1);One();CHECK(Save_Write(0,"modern controls",true));
    CT_Reset(CT_CLASSIC);CHECK(Save_Load(0,ticks));CHECK(CT_Preset()==CT_CLASSIC && !CT_MouseEnabled());
    CHECK(Save_GetAngleFraction()==17);CT_Reset(CT_MODERN);CT_SetSensitivity(17);
    CT_Key(SDL_SCANCODE_W,true);CT_MouseMotion(1);One();CHECK(Save_Write(1,"continuous",true));rng=US_RndIndex();
    CHECK(Save_Load(0,ticks-1));--ticks;CT_Key(SDL_SCANCODE_W,true);CT_MouseMotion(1);One();
    CHECK(Save_Write(2,"restored",true) && US_RndIndex()==rng);
    puts("Classic/Modern movement, normalization, mouse, bindings and deterministic saves passed.");
}
static void Event(SDL_EventType type)
{ SDL_Event e={0};e.type=type;CHECK(SDL_PushEvent(&e)); }
static void Key(SDL_Scancode code,bool down)
{ SDL_Event e={0};e.type=down?SDL_EVENT_KEY_DOWN:SDL_EVENT_KEY_UP;e.key.scancode=code;CHECK(SDL_PushEvent(&e)); }
static void Step(unsigned n) { while(n--) { CHECK(!VL_PollQuit());CHECK(DemoLoop(++ticks));CHECK(VW_UpdateScreen()); } }
static void Tap(SDL_Scancode key) { Key(key,true);Key(key,false);Step(1); }
static void Mouse(unsigned button,bool down)
{ SDL_Event e={0};e.type=down?SDL_EVENT_MOUSE_BUTTON_DOWN:SDL_EVENT_MOUSE_BUTTON_UP;e.button.button=(uint8_t)button;CHECK(SDL_PushEvent(&e)); }
static void Capture(const char *name)
{
    const char *dir=getenv("WOLF3D_CAPTURE_DIR");if(!dir) return;
    char path[4096];(void)snprintf(path,sizeof(path),"%s/%s.bmp",dir,name);CHECK(VL_Capture(path));
}
static void SDLControls(void)
{
    CHECK(VL_Startup());CT_Reset(CT_CLASSIC);CHECK(DemoLoopStartup(ticks,true));Step(11);CHECK(!IN_SDLCaptured());
    MainItems.curpos=control;Tap(SDL_SCANCODE_RETURN);Step(21);CHECK(DemoLoopState()==UI_CONTROLS);Capture("controls-classic");
    Tap(SDL_SCANCODE_RETURN);CHECK(CT_Preset()==CT_MODERN && CT_MouseEnabled());Capture("controls-modern");
    for(unsigned i=0;i<5;++i) Tap(SDL_SCANCODE_DOWN);
    Tap(SDL_SCANCODE_RETURN);Capture("controls-bindings");Tap(SDL_SCANCODE_RETURN);Tap(SDL_SCANCODE_S);
    CHECK(CT_Binding(CT_FORWARD,0)==SDL_SCANCODE_W);Capture("controls-conflict");Tap(SDL_SCANCODE_E);
    CHECK(CT_Binding(CT_FORWARD,0)==SDL_SCANCODE_E);Tap(SDL_SCANCODE_ESCAPE);Tap(SDL_SCANCODE_ESCAPE);Step(21);
    CHECK(DemoLoopState()==UI_MENU);Mouse(1,true);Step(1);CHECK(!IN_SDLCaptured());
    CHECK(DemoLoopStartGame(ticks));Step(11);Arena();CHECK(IN_SDLCaptured());
    CHECK(!CT_Down(CT_FIRE));
    tilemap[11][10]=128;doornum=1;doorobjlist[0]=(doorobj_t){.x=11,.y=10,.vertical=true};
    Mouse(3,true);Step(1);CHECK(doorobjlist[0].action==dr_opening);
    Step(1);CHECK(doorobjlist[0].position>0 && doorobjlist[0].action==dr_opening);
    Mouse(3,false);Step(1);Arena();
    Mouse(1,false);Mouse(1,true);Step(1);CHECK(gamestate.attackframe==1);Mouse(1,false);Step(1);
    int angle=player.angle;SDL_Event motion={0};motion.type=SDL_EVENT_MOUSE_MOTION;motion.motion.xrel=4;
    CHECK(SDL_PushEvent(&motion));Step(1);CHECK(player.angle==(angle+358)%360);
    Event(SDL_EVENT_WINDOW_FOCUS_LOST);Step(1);CHECK(!IN_SDLCaptured() && !CT_Down(CT_FIRE));
    int32_t x=player.x;Key(SDL_SCANCODE_E,true);Step(1);CHECK(player.x==x);
    Event(SDL_EVENT_WINDOW_FOCUS_GAINED);Step(1);CHECK(IN_SDLCaptured());
    Key(SDL_SCANCODE_E,true);Step(1);CHECK(player.x>x);Key(SDL_SCANCODE_E,false);Step(1);
    CHECK(CT_Bind(CT_MENU,1,SDL_SCANCODE_P));Tap(SDL_SCANCODE_P);Step(21);CHECK(DemoLoopState()==UI_MENU && !IN_SDLCaptured());
    uint32_t time=gamestate.TimeCount;Mouse(1,true);Key(SDL_SCANCODE_E,true);Step(70);CHECK(gamestate.TimeCount==time);
    MainItems.curpos=backtodemo;Tap(SDL_SCANCODE_RETURN);Step(21);CHECK(DemoLoopState()==UI_GAME && IN_SDLCaptured());
    CHECK(!CT_Down(CT_FIRE) && !CT_Down(CT_FORWARD));
    Mouse(1,false);Key(SDL_SCANCODE_E,false);Step(1);GameStop();CHECK(VW_UpdateScreen());CHECK(!IN_SDLCaptured());VL_Shutdown();
    puts("SDL controls menu, conflicts, relative capture, mouse/keys, focus and pause/resume passed.");
}
int main(int argc,char **argv)
{
    CHECK(argc==4);MM_Startup();CHECK(CA_Startup(argv[1]));CHECK(VL_SetPalette(gamepal,768));CHECK(Save_SetDirectory(argv[2]));
    if(!strcmp(argv[3],"core")) Core();else SDLControls();
    GameStop();CA_Shutdown();MM_Shutdown();CHECK(!MM_TotalBytes());return 0;
}
