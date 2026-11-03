/* SPDX-License-Identifier: GPL-2.0-only */
#include "engine/WL_DEF.h"
#include "engine/WL_DEMO.h"
#include "engine/WL_MAIN.h"
#include "engine/WL_MENU.h"
#include "engine/WL_SAVE.h"
#include "engine/WL_CONTROLS.h"
#include "engine/WL_DISPLAY.h"
#include "engine/ID_CA.h"
#include "engine/ID_IN.h"
#include "engine/ID_VH.h"
#include "engine/ID_US_1.h"
#include "engine/ID_SD.h"
#include "platform/sdl_video.h"
#include "platform/sdl_input.h"
#include <SDL3/SDL.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(x) do { if(!(x)) { fprintf(stderr,"%d: %s (%s)\n",__LINE__,#x,CA_Error());exit(1); } } while(0)
static uint64_t ticks;
static void Step(unsigned n) { while(n--) { CHECK(!VL_PollQuit());CHECK(DemoLoop(++ticks));CHECK(VW_UpdateScreen()); } }
static void Tap(SDL_Scancode key) { SDL_Event e={0};e.type=SDL_EVENT_KEY_DOWN;e.key.scancode=key;CHECK(SDL_PushEvent(&e));e.type=SDL_EVENT_KEY_UP;CHECK(SDL_PushEvent(&e));Step(1); }
static void Start(void) { CHECK(NewViewSize(15));CHECK(DemoLoopStartup(ticks,true));CHECK(DemoLoopStartGame(ticks));Step(10);CHECK(DemoLoopState()==UI_GAME); }
static void FKey(SDL_Scancode key,ui_state state) { Tap(key);Step(20);CHECK(DemoLoopState()==state); }
static void Back(void) { Tap(SDL_SCANCODE_ESCAPE);Step(20);CHECK(DemoLoopState()==UI_GAME); }
static void Capture(const char *name) { const char *dir=getenv("WOLF3D_CAPTURE_DIR");if(dir) { char p[4096];(void)snprintf(p,sizeof(p),"%s/%s.bmp",dir,name);CHECK(VL_Capture(p)); } }
static void Border(void)
{
    for(int y=0;y<160;++y) for(int x=0;x<320;++x) {
        if(x>=VIEWX && x<VIEWX+VIEWWIDTH && y>=VIEWY && y<VIEWY+VIEWHEIGHT) continue;
        unsigned color=127;
        if((x==VIEWX-1 || y==VIEWY-1) && x>=VIEWX-1 && x<=VIEWX+VIEWWIDTH && y>=VIEWY-1 && y<=VIEWY+VIEWHEIGHT) color=0;
        if((x==VIEWX+VIEWWIDTH || y==VIEWY+VIEWHEIGHT) && x>=VIEWX-1 && x<=VIEWX+VIEWWIDTH && y>=VIEWY-1 && y<=VIEWY+VIEWHEIGHT) color=125;
        /* Original top-right is overwritten by the right vertical bevel. */
        if(x==VIEWX-1 && y==VIEWY+VIEWHEIGHT) color=124;
        CHECK(framebuffer[y*320+x]==color);
    }
}
static void Views(void)
{
    CHECK(GameStart(0));CHECK(Save_Write(0,"before view",true));
    for(unsigned size=4;size<=19;++size) {
        unsigned rng=US_RndIndex();uint32_t time=gamestate.TimeCount;
        CHECK(NewViewSize(size));CHECK(ViewSize()==size && VIEWWIDTH==size*16 && VIEWHEIGHT==size*8);
        CHECK(VIEWX==(320-VIEWWIDTH)/2 && VIEWY==(160-VIEWHEIGHT)/2);
        CHECK(GameDrawPaused());Border();
        CHECK(US_RndIndex()==rng && gamestate.TimeCount==time);
        for(int i=0;i<VIEWWIDTH/2;++i) {
            double tangent=(double)(i*65536/VIEWWIDTH)/(FOCALLENGTH+MINDIST);
            int expected=(int)((float)atan(tangent)*(float)(3600/2/3.141592657));
            CHECK(pixelangle[VIEWWIDTH/2-1-i]==expected && pixelangle[VIEWWIDTH/2+i]==-expected);
        }
        w3r_byte hud[320*40];memcpy(hud,framebuffer+320*160,sizeof(hud));
        CHECK(ThreeDRefreshPaused());CHECK(!memcmp(hud,framebuffer+320*160,sizeof(hud)));Border();
        /* Oversize wall and near sprite clipping must never touch the border. */
        w3r_byte column[64];memset(column,4,sizeof(column));
        ScalePost(0,VIEWWIDTH,UINT32_MAX,column);CHECK(SimpleScaleShape(VIEWWIDTH/2,SPR_PISTOLREADY,VIEWHEIGHT+1));Border();
    }
    CHECK(Save_Write(1,"after view",true));CHECK(Save_Load(0,0));CHECK(ViewSize()==19);
    CHECK(!NewViewSize(3) && !NewViewSize(20) && ViewSize()==19);
    for(unsigned size=4;size<=19;++size) {
        CHECK(NewViewSize(size));CHECK(GameStart(0));gamestate.died=true;killerobj=NULL;
        for(uint64_t t=1;t<=200;++t) CHECK(GameStep(t));
        Border(); /* Death fizzle stays inside every supported viewport. */
    }
    CHECK(NewViewSize(15));GameStop();puts("All 16 DOS viewport sizes, projection, teal bevel, sprite/HUD clipping, death and save isolation passed.");
}
static void Keys(const char *slots)
{
    /* Exact historical strings and precedence, including the ninth saying. */
    CHECK(!strcmp(MenuQuitMessage(8,1),"For guns and glory, press N.\nFor work and worry, press Y."));
    CHECK(!strcmp(MenuQuitMessage(1,0),"Chickening out...\nalready?"));
    for(unsigned i=0;i<9;++i) CHECK(BeginConfirmation(MenuQuitMessage(i,i==8?1:0),ticks));
    unsigned original_rng=US_RndIndex();
    CHECK(BeginConfirmation("Confirm?",ticks));
    w3r_byte initial[VL_PIXELS];memcpy(initial,framebuffer,sizeof(initial));
    CHECK(HandleConfirmation(ticks+10)==0 && !memcmp(initial,framebuffer,sizeof(initial)));
    CHECK(HandleConfirmation(ticks+20)==0 && memcmp(initial,framebuffer,sizeof(initial)));
    CHECK(HandleConfirmation(ticks+30)==0 && !memcmp(initial,framebuffer,sizeof(initial)));
    IN_KeyEvent(sc_Y,true);CHECK(HandleConfirmation(ticks+31)==0);
    IN_KeyEvent(sc_Y,false);CHECK(HandleConfirmation(ticks+32)==1 && US_RndIndex()==original_rng);
    IN_KeyEvent(sc_N,true);IN_KeyEvent(sc_N,false);CHECK(HandleConfirmation(ticks+33)==-1);
    /* A fresh process has no save directory; unavailable F2/F3/F8/F9 are safe. */
    Start();const SDL_Scancode unavailable[]={SDL_SCANCODE_F2,SDL_SCANCODE_F3,SDL_SCANCODE_F8,SDL_SCANCODE_F9};
    for(unsigned i=0;i<4;++i) { Tap(unavailable[i]);CHECK(DemoLoopState()==UI_GAME); }
    CHECK(Save_SetDirectory(slots));
    const ui_state panels[]={UI_BOSS,UI_SAVE,UI_LOAD,UI_SOUND,UI_VIEW,UI_CONTROLS};
    for(unsigned i=0;i<6;++i) {
        Start();uint32_t time=gamestate.TimeCount;unsigned rng=US_RndIndex();
        FKey((SDL_Scancode)(SDL_SCANCODE_F1+i),panels[i]);
        CHECK(gamestate.TimeCount==time && US_RndIndex()==rng);Step(70);CHECK(gamestate.TimeCount==time);
        if(i==0) { unsigned m,e;bool on,fx;SD_GetSettings(&m,&e,&on,&fx);CHECK(!on);Tap(SDL_SCANCODE_SPACE);CHECK(DemoLoopState()==UI_BOSS); }
        if(i==4) { Capture("classic-change-view");Tap(SDL_SCANCODE_LEFT);CHECK(ViewSize()==15); }
        Back();CHECK(gamestate.TimeCount==time && ViewSize()==15);
    }
    FKey(SDL_SCANCODE_F5,UI_VIEW);
    for(unsigned i=0;i<25;++i) Tap(SDL_SCANCODE_LEFT);
    for(unsigned i=0;i<25;++i) Tap(SDL_SCANCODE_UP);
    Tap(SDL_SCANCODE_RETURN);Step(20);CHECK(ViewSize()==19 && DemoLoopState()==UI_GAME);
    FKey(SDL_SCANCODE_F5,UI_VIEW);Tap(SDL_SCANCODE_DOWN);Tap(SDL_SCANCODE_RETURN);Step(20);CHECK(ViewSize()==18);
    CHECK(NewViewSize(15));
    FKey(SDL_SCANCODE_F5,UI_VIEW);Tap(SDL_SCANCODE_RIGHT);Tap(SDL_SCANCODE_RETURN);Step(20);CHECK(DemoLoopState()==UI_GAME && ViewSize()==16);Border();Capture("classic-size16");
    for(unsigned key=58;key<=68;++key) CHECK(!CT_Bind(CT_FIRE,0,key));
    Start();uint32_t paused_time=gamestate.TimeCount;unsigned paused_rng=US_RndIndex();
    Tap(SDL_SCANCODE_PAUSE);CHECK(DemoLoopState()==UI_PAUSE && !CT_Gameplay());
    Step(140);CHECK(gamestate.TimeCount==paused_time && US_RndIndex()==paused_rng);
    Tap(SDL_SCANCODE_SPACE);CHECK(DemoLoopState()==UI_GAME && gamestate.TimeCount==paused_time);
    Tap(SDL_SCANCODE_ESCAPE);Step(20);MainItems.curpos=newgame;
    Tap(SDL_SCANCODE_RETURN);Step(20);CHECK(DemoLoopState()==UI_EPISODE);
    Tap(SDL_SCANCODE_RETURN);CHECK(DemoLoopState()==UI_QUICK);
    Tap(SDL_SCANCODE_N);Step(20);CHECK(DemoLoopState()==UI_EPISODE && GameRunning());
    Tap(SDL_SCANCODE_RETURN);Tap(SDL_SCANCODE_Y);Step(20);CHECK(DemoLoopState()==UI_DIFFICULTY);
    CHECK(gamestate.TimeCount==paused_time && US_RndIndex()==paused_rng);
    Start();FKey(SDL_SCANCODE_F2,UI_SAVE);Tap(SDL_SCANCODE_RETURN);Tap(SDL_SCANCODE_RETURN);Step(20);CHECK(DemoLoopState()==UI_GAME);
    gamestate.ammo=2;FKey(SDL_SCANCODE_F3,UI_LOAD);Tap(SDL_SCANCODE_RETURN);Step(20);CHECK(DemoLoopState()==UI_GAME && gamestate.ammo==8);
    char initial_path[4096];(void)snprintf(initial_path,sizeof(initial_path),"%s/slot1.w3s",slots);CHECK(remove(initial_path)==0);
    CHECK(!GameStartSelected(ticks,UINT32_MAX,2) && !GameRunning());
    Start();gamestate.died=true;Tap(SDL_SCANCODE_F2);CHECK(DemoLoopState()==UI_GAME && !GameCanSave());
    Start();FKey(SDL_SCANCODE_F8,UI_SAVE);Tap(SDL_SCANCODE_RETURN);Tap(SDL_SCANCODE_RETURN);Step(20);CHECK(DemoLoopState()==UI_GAME);
    gamestate.ammo=3;Tap(SDL_SCANCODE_F8);CHECK(DemoLoopState()==UI_GAME);
    gamestate.ammo=7;Tap(SDL_SCANCODE_F9);CHECK(DemoLoopState()==UI_QUICK);Tap(SDL_SCANCODE_N);Step(20);CHECK(DemoLoopState()==UI_GAME && gamestate.ammo==7);
    Tap(SDL_SCANCODE_F9);Tap(SDL_SCANCODE_Y);Step(20);CHECK(DemoLoopState()==UI_GAME && gamestate.ammo==3);
    /* Revalidate on confirmation: corrupting the selected file must retain game. */
    Tap(SDL_SCANCODE_F9);char path[4096];(void)snprintf(path,sizeof(path),"%s/slot1.w3s",slots);FILE *f=fopen(path,"wb");CHECK(f);CHECK(fclose(f)==0);
    Tap(SDL_SCANCODE_Y);CHECK(DemoLoopState()==UI_QUICK && GameRunning());Tap(SDL_SCANCODE_SPACE);Step(20);CHECK(DemoLoopState()==UI_GAME && gamestate.ammo==3);
    Tap(SDL_SCANCODE_F7);CHECK(DemoLoopState()==UI_QUICK);Tap(SDL_SCANCODE_N);Step(20);CHECK(GameRunning() && DemoLoopState()==UI_GAME);
    Tap(SDL_SCANCODE_F10);CHECK(DemoLoopState()==UI_QUICK);Tap(SDL_SCANCODE_ESCAPE);Step(20);CHECK(GameRunning() && DemoLoopState()==UI_GAME);
    Tap(SDL_SCANCODE_F7);Tap(SDL_SCANCODE_Y);Step(20);CHECK(!GameRunning() && DemoLoopState()==UI_HIGHSCORES);
    Start();Tap(SDL_SCANCODE_F10);Tap(SDL_SCANCODE_Y);Step(20);CHECK(DemoLoopState()==UI_DONE);
    GameStop();puts("F1-F10 panels, direct return, pause, Change View accept/cancel, GOODTIMES boss key, quicksave/load/errors and end/quit passed.");
}
static uint32_t FrameHash(void) { uint32_t h=2166136261U;for(size_t i=0;i<VL_PIXELS;++i) h=(h^framebuffer[i])*16777619U;return h; }
static void Mouse(bool down) { SDL_Event e={0};e.type=down?SDL_EVENT_MOUSE_BUTTON_DOWN:SDL_EVENT_MOUSE_BUTTON_UP;e.button.button=1;CHECK(SDL_PushEvent(&e)); }
static void Missing(void)
{
    for(unsigned i=0;i<5;++i) { CHECK(!Demo_Start(i,0));CHECK(!Demo_Active() && !GameRunning() && SimulationTics()==1); }
    CHECK(DemoLoopStartup(0,false));CHECK(DemoLoop(2660));ticks=2660;CHECK(DemoLoopState()==UI_TITLE);
    Tap(SDL_SCANCODE_SPACE);Step(60);CHECK(DemoLoopState()==UI_MENU);
    puts("Missing/corrupt demos skip safely to title; no fake playback or live game remains.");
}
static void Demos(void)
{
    demo_progress progress;CHECK(!Demo_Progress(NULL) && !Demo_Progress(&progress));
    unsigned map;size_t len;w3r_byte data[]={0,7,0,216,0,0,0};
    CHECK(Demo_Validate(data,sizeof(data),&map,&len) && map==0 && len==7);
    for(size_t n=0;n<7;++n) CHECK(!Demo_Validate(data,n,&map,&len));
    data[0]=60;CHECK(!Demo_Validate(data,7,&map,&len));data[0]=0;
    data[1]=8;CHECK(!Demo_Validate(data,7,&map,&len));data[1]=7;
    data[5]=127;CHECK(!Demo_Validate(data,7,&map,&len));
    for(unsigned n=0;n<4;++n) {
        CHECK(CA_CacheGrChunk(T_DEMO0+(int)n));CHECK(Demo_Validate(grsegs[T_DEMO0+n],grsizes[T_DEMO0+n],&map,&len));
        unsigned rng=0;int32_t x=0,y=0;int angle=0;uint32_t time=0,hash=0;
        for(unsigned variant=0;variant<2;++variant) {
            CT_Reset(variant?CT_MODERN:CT_CLASSIC);CHECK(Demo_Start(n,100));CHECK(!GameCanSave());
            CHECK(Demo_Progress(&progress) && progress.map==map && progress.commands_total==(len-4)/3 && progress.commands_consumed==0 && progress.buttons==0);
            CHECK(!Demo_Progress(NULL));
            unsigned initial_rng=US_RndIndex();
            CHECK(Demo_Progress(&progress) && US_RndIndex()==initial_rng);
            CT_Key(26,true);CT_Key(224,true);CT_MouseMotion(123);
            uint64_t t=100;
            while(!Demo_Finished()) { t+=variant?4U:1U;CHECK(GameStep(t));CHECK(t<=100+4*(len-4)/3+4); }
            CHECK(gamestate.TimeCount<=4*(len-4)/3);
            if(map==0) CHECK(gamestate.TimeCount==4*(len-4)/3);
            if(!variant) { rng=US_RndIndex();x=player.x;y=player.y;angle=player.angle;time=gamestate.TimeCount;hash=FrameHash(); }
            else CHECK(US_RndIndex()==rng && player.x==x && player.y==y && player.angle==angle && gamestate.TimeCount==time && FrameHash()==hash);
            CHECK(Demo_Progress(&progress) && progress.commands_consumed==gamestate.TimeCount/4 && progress.buttons==Demo_Buttons());
            GameStop();CHECK(!Demo_Active() && !Demo_Progress(&progress));
        }
        printf("Demo %u validated; deterministic playback %u tics of %zu.\n",n,time,4*(len-4)/3);
    }
    CT_Reset(CT_CLASSIC);ticks=0;CHECK(DemoLoopStartup(0,false));CHECK(DemoLoop(2660));ticks=2660;CHECK(DemoLoopState()==UI_DEMO && !CT_Gameplay());
    Tap(SDL_SCANCODE_SPACE);Step(60);CHECK(DemoLoopState()==UI_MENU && !Demo_Active() && !GameRunning());
    ticks=0;CHECK(DemoLoopStartup(0,false));CHECK(DemoLoop(2660));ticks=2660;
    SDL_Event focus={0};focus.type=SDL_EVENT_WINDOW_FOCUS_LOST;CHECK(SDL_PushEvent(&focus));Mouse(true);Step(20);CHECK(DemoLoopState()==UI_DEMO);
    focus.type=SDL_EVENT_WINDOW_FOCUS_GAINED;CHECK(SDL_PushEvent(&focus));Mouse(false);Mouse(true);Mouse(false);Step(60);CHECK(DemoLoopState()==UI_MENU && !Demo_Active());
    puts("Original demo decoding, bounds, duration, deterministic cadence, input isolation and attract abort passed.");
}
int main(int argc,char **argv)
{
    CHECK(argc==4);MM_Startup();CHECK(CA_Startup(argv[1]));CHECK(VL_Startup());CHECK(VL_SetPalette(gamepal,768));Display_Connect(VL_DisplayGet,VL_DisplaySet);
    if(!strcmp(argv[3],"keys")) Keys(argv[2]);
    else { CHECK(Save_SetDirectory(argv[2]));if(!strcmp(argv[3],"view")) Views();else if(!strcmp(argv[3],"missing") || !strcmp(argv[3],"corrupt")) Missing();else Demos(); }
    GameStop();Display_Connect(NULL,NULL);VL_Shutdown();CA_Shutdown();MM_Shutdown();CHECK(!MM_TotalBytes());return 0;
}
