/* SPDX-License-Identifier: GPL-2.0-only */
#include "engine/WL_DEF.h"
#include "engine/WL_DEMO.h"
#include "engine/WL_MAIN.h"
#include "engine/WL_MENU.h"
#include "engine/WL_SAVE.h"
#include "engine/WL_HIGHSCORES.h"
#include "engine/WL_CONTROLS.h"
#include "engine/WL_DISPLAY.h"
#include "engine/ID_CA.h"
#include "engine/ID_IN.h"
#include "engine/ID_VH.h"
#include "engine/ID_US_1.h"
#include "engine/ID_SD.h"
#include "platform/sdl_video.h"
#include "platform/sdl_input.h"
#include "platform/sdl_input.h"
#include <SDL3/SDL.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(x) do { if(!(x)) { fprintf(stderr,"%d: %s (%s)\n",__LINE__,#x,CA_Error());exit(1); } } while(0)
#include "finish_loading.h"
static uint64_t ticks;
static void Step(unsigned n) { while(n--) { CHECK(!VL_PollQuit());CHECK(DemoLoop(++ticks));if(GameLoading()) TestFinishLoading(ticks);CHECK(VW_UpdateScreen()); } }
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
    Tap(SDL_SCANCODE_PAUSE);CHECK(DemoLoopState()==UI_PAUSE && !CT_Gameplay() && SD_MusicPaused());
    Step(140);CHECK(gamestate.TimeCount==paused_time && US_RndIndex()==paused_rng);
    Tap(SDL_SCANCODE_SPACE);CHECK(DemoLoopState()==UI_GAME && gamestate.TimeCount==paused_time && !SD_MusicPaused());
    Tap(SDL_SCANCODE_ESCAPE);Step(20);MainMenu[newgame].active=1;MainItems.curpos=newgame;
    Tap(SDL_SCANCODE_RETURN);Step(20);CHECK(DemoLoopState()==UI_EPISODE);
    Tap(SDL_SCANCODE_RETURN);CHECK(DemoLoopState()==UI_QUICK);
    Tap(SDL_SCANCODE_N);Step(20);CHECK(DemoLoopState()==UI_MENU && GameRunning());
    Tap(SDL_SCANCODE_RETURN);Step(20);CHECK(DemoLoopState()==UI_EPISODE);
    Tap(SDL_SCANCODE_RETURN);Tap(SDL_SCANCODE_Y);Step(20);CHECK(DemoLoopState()==UI_DIFFICULTY);
    CHECK(gamestate.TimeCount==paused_time && US_RndIndex()==paused_rng);
    Start();FKey(SDL_SCANCODE_F2,UI_SAVE);Tap(SDL_SCANCODE_RETURN);Tap(SDL_SCANCODE_RETURN);Step(22);CHECK(DemoLoopState()==UI_GAME);
    gamestate.ammo=2;FKey(SDL_SCANCODE_F3,UI_LOAD);Tap(SDL_SCANCODE_RETURN);Step(22);CHECK(DemoLoopState()==UI_GAME && gamestate.ammo==8);
    char initial_path[4096];(void)snprintf(initial_path,sizeof(initial_path),"%s/slot1.w3s",slots);CHECK(remove(initial_path)==0);
    CHECK(!GameStartSelected(ticks,UINT32_MAX,2) && !GameRunning());
    Start();gamestate.died=true;Tap(SDL_SCANCODE_F2);CHECK(DemoLoopState()==UI_GAME && !GameCanSave());
    Start();FKey(SDL_SCANCODE_F8,UI_SAVE);Tap(SDL_SCANCODE_RETURN);Tap(SDL_SCANCODE_RETURN);Step(22);CHECK(DemoLoopState()==UI_GAME);
    gamestate.ammo=3;Tap(SDL_SCANCODE_F8);CHECK(DemoLoopState()==UI_DISK);Step(2);CHECK(DemoLoopState()==UI_GAME);
    gamestate.ammo=7;Tap(SDL_SCANCODE_F9);CHECK(DemoLoopState()==UI_QUICK);Tap(SDL_SCANCODE_N);Step(20);CHECK(DemoLoopState()==UI_GAME && gamestate.ammo==7);
    Tap(SDL_SCANCODE_F9);Tap(SDL_SCANCODE_Y);Step(20);CHECK(DemoLoopState()==UI_GAME && gamestate.ammo==3);
    /* Revalidate on confirmation: corrupting the selected file must retain game. */
    Tap(SDL_SCANCODE_F9);char path[4096];(void)snprintf(path,sizeof(path),"%s/slot1.w3s",slots);FILE *f=fopen(path,"wb");CHECK(f);CHECK(fclose(f)==0);
    Tap(SDL_SCANCODE_Y);Step(1);CHECK(DemoLoopState()==UI_QUICK && GameRunning());Tap(SDL_SCANCODE_SPACE);Step(20);CHECK(DemoLoopState()==UI_GAME && gamestate.ammo==3);
    Tap(SDL_SCANCODE_F7);CHECK(DemoLoopState()==UI_QUICK);Tap(SDL_SCANCODE_N);Step(20);CHECK(GameRunning() && DemoLoopState()==UI_GAME);
    Tap(SDL_SCANCODE_F10);CHECK(DemoLoopState()==UI_QUICK);Tap(SDL_SCANCODE_ESCAPE);Step(20);CHECK(GameRunning() && DemoLoopState()==UI_GAME);
    Tap(SDL_SCANCODE_F7);Tap(SDL_SCANCODE_Y);Step(20);CHECK(gamestate.died && gamestate.lives==0);
    for(unsigned i=0;i<600 && DemoLoopState()==UI_GAME;++i) Step(1);
    CHECK(!GameRunning() && DemoLoopState()==UI_RESULTS);
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
static void Interface(void)
{
    Start();uint32_t time=gamestate.TimeCount;unsigned rng=US_RndIndex();
    Tap(SDL_SCANCODE_ESCAPE);Step(20);CHECK(DemoLoopState()==UI_MENU);
    CT_SetMouse(true);MainItems.curpos=changeview;CHECK(DrawMainMenu());
    IN_SDLPointerMotion(NAN,0);IN_SDLPointerMotion(0,INFINITY);Step(1);CHECK(MainItems.curpos==changeview);
    IN_SDLPointerMotion(0,60);Step(1);CHECK(MainItems.curpos==changeview);
    IN_SDLPointerMotion(0,1);Step(1);CHECK(MainItems.curpos==viewscores);
    IN_SDLPointerMotion(61,-100);Step(1);CHECK(MainItems.curpos==viewscores); /* Horizontal wins. */
    IN_SDLMouseButton(SDL_BUTTON_RIGHT,true);IN_SDLMouseButton(SDL_BUTTON_RIGHT,false);Step(1);CHECK(DemoLoopState()==UI_CONFIRM);
    IN_SDLMouseButton(SDL_BUTTON_LEFT,true);IN_SDLMouseButton(SDL_BUTTON_LEFT,false);Step(1);CHECK(DemoLoopState()==UI_CONFIRM);
    Tap(SDL_SCANCODE_N);CHECK(DemoLoopState()==UI_MENU);
    unsigned m,e;bool on,fx;SD_GetSettings(&m,&e,&on,&fx);
    Tap(SDL_SCANCODE_PAUSE);bool changed,ef;SD_GetSettings(&m,&e,&changed,&ef);CHECK(changed==on && ef==fx && SD_MusicPaused());
    Tap(SDL_SCANCODE_PAUSE);SD_GetSettings(&m,&e,&changed,&ef);CHECK(changed==on && !SD_MusicPaused());
    IN_ClearKeysDown();IN_MenuKeyEvent(0x2a,true);IN_MenuKeyEvent(0x1e,true);IN_MenuKeyEvent(0x1e,false);
    IN_MenuKeyEvent(0x02,true);IN_MenuKeyEvent(0x02,false);IN_MenuKeyEvent(0x2a,false);
    CHECK(IN_NextASCII()=='A' && IN_NextASCII()=='!' && !IN_NextASCII());
    IN_MenuKeyEvent(0x3a,true);IN_MenuKeyEvent(0x3a,false);IN_MenuKeyEvent(0x30,true);IN_MenuKeyEvent(0x30,false);
    CHECK(IN_NextASCII()=='B');IN_MenuKeyEvent(0x3a,true);IN_MenuKeyEvent(0x3a,false);IN_ClearKeysDown();
    CHECK(gamestate.TimeCount==time && US_RndIndex()==rng);
    CHECK(DrawSaveActivity(true,false));w3r_byte disk[VL_PIXELS];memcpy(disk,framebuffer,sizeof(disk));
    CHECK(DrawSaveActivity(true,true) && memcmp(disk,framebuffer,sizeof(disk)));
    CT_SetMouse(false);
    /* A held native key repeats in DOS name entry, but never confirms a page. */
    Start();gamestate.lives=0;gamestate.score=20000;gamestate.died=true;
    for(unsigned i=0;i<600 && DemoLoopState()==UI_GAME;++i) Step(1);
    CHECK(DemoLoopState()==UI_RESULTS);Step(40);
    IN_SDLKey(SDL_SCANCODE_A,true);IN_SDLRepeat(SDL_SCANCODE_A);IN_SDLRepeat(SDL_SCANCODE_A);IN_SDLKey(SDL_SCANCODE_A,false);Step(1);
    Tap(SDL_SCANCODE_RETURN);CHECK(!strcmp(HighScores_Table()[0].name,"aaa"));
    GameStop();
    puts("DOS mouse threshold/priority/buttons, confirmation isolation, menu Pause, shifted text and disk artwork passed.");
}
static void Chord(unsigned a,unsigned b,unsigned c)
{ IN_KeyEvent(a,true);IN_KeyEvent(b,true);if(c) IN_KeyEvent(c,true);Step(1); }
static void Hidden(void)
{
    CHECK(SD_Startup(48000));
    Start();unsigned rng=US_RndIndex();uint32_t time=gamestate.TimeCount;
    gamestate.score=777;gamestate.health=1;gamestate.ammo=1;
    Chord(0x32,0x26,0x17);CHECK(DemoLoopState()==UI_MESSAGE);
    CHECK(gamestate.score==0 && gamestate.health==100 && gamestate.ammo==99 && gamestate.keys==3 && gamestate.bestweapon==wp_chaingun);
    CHECK(gamestate.TimeCount==time+42000 && US_RndIndex()==rng);
    Step(70);CHECK(gamestate.TimeCount==time+42000);
    Tap(SDL_SCANCODE_SPACE);CHECK(DemoLoopState()==UI_GAME && gamestate.TimeCount==time+42000);
    Chord(0x30,0x1e,0x14);CHECK(DemoLoopState()==UI_MESSAGE && US_RndIndex()==rng);
    Tap(SDL_SCANCODE_RETURN);CHECK(DemoLoopState()==UI_GAME);
    DemoLoopAllowDebug(true);Chord(0x0e,0x2a,0x38);CHECK(DemoLoopState()==UI_MESSAGE);
    Tap(SDL_SCANCODE_RETURN);Chord(0x0f,0x22,0);CHECK(DemoLoopState()==UI_MESSAGE && GameGodMode());
    TakeDamage(16,NULL);CHECK(gamestate.health==100);
    Tap(SDL_SCANCODE_RETURN);Chord(0x0f,0x22,0);CHECK(!GameGodMode());Tap(SDL_SCANCODE_RETURN);
    Chord(0x0f,0x23,0);CHECK(gamestate.health==84);
    gamestate.oldscore=500;gamestate.score=999;
    Chord(0x0f,0x11,0);CHECK(DemoLoopState()==UI_WARP);Step(70);CHECK(gamestate.score==999);
    Tap(SDL_SCANCODE_2);Tap(SDL_SCANCODE_RETURN);CHECK(DemoLoopState()==UI_GAME && gamestate.mapon==1);
    CHECK(gamestate.score==500 && gamestate.health==84 && gamestate.TimeCount==0);
    CHECK(!GameWarp(ticks,0) && !GameWarp(ticks,11) && gamestate.mapon==1);
    DemoLoopAllowDebug(false);
    GameStop();
    static const int first[]={3,8,6};
    for(unsigned bank=0;bank<3;++bank) {
        CHECK(DemoLoopJukebox(ticks,bank));Step(10);Tap(SDL_SCANCODE_RETURN);
        sd_status status;SD_GetStatus(&status);CHECK(status.music==first[bank]);
        uint32_t stopped=gamestate.TimeCount;Step(70);CHECK(gamestate.TimeCount==stopped);
        Tap(SDL_SCANCODE_DOWN);Tap(SDL_SCANCODE_RETURN);
        Tap(SDL_SCANCODE_ESCAPE);Step(40);CHECK(DemoLoopState()==UI_TITLE);
    }
    CHECK(!DemoLoopJukebox(ticks,3));
    Start();Tap(SDL_SCANCODE_F10);Tap(SDL_SCANCODE_Y);
    sd_status quit_audio;SD_GetStatus(&quit_audio);CHECK(quit_audio.music==-1 && quit_audio.sound==-1 && quit_audio.digitized==-1);
    Step(20);CHECK(DemoLoopState()==UI_DONE);GameStop();SD_Shutdown();
    puts("MLI penalty/inventory, BAT, debug unlock/god/hurt and all jukebox banks passed without simulation or RNG leakage.");
}
static void Scores(const char *directory,bool reading)
{
    CHECK(HighScores_Load(directory));
    if(reading) {
        const high_score *t=HighScores_Table();
        CHECK(t[0].score==123456 && !strcmp(t[0].name,"acb") && t[0].episode==5 && t[0].completed==9);
        puts("High scores survived a process restart.");return;
    }
    unsigned rng=US_RndIndex();
    CHECK(HighScores_Insert(9999,10,0)==-1 && HighScores_Insert(10000,1,0)==-1);
    CHECK(HighScores_Insert(10000,2,3)==0 && HighScores_Table()[0].episode==3);
    CHECK(HighScores_Insert(10000,2,2)==1); /* Exact ties do not displace. */
    CHECK(HighScores_Insert(UINT32_MAX,11,0)==-1 && HighScores_Insert(1,1,6)==-1);
    HighScores_Reset();CHECK(HighScores_Insert(123456,9,5)==0);
    CHECK(BeginHighScoreEntry(0,0));
    IN_KeyEvent(0x1e,true);IN_KeyEvent(0x1e,false); /* a */
    IN_KeyEvent(0x30,true);IN_KeyEvent(0x30,false); /* b */
    CHECK(HandleHighScoreEntry(1)==0);
    IN_KeyEvent(sc_LeftArrow,true);IN_KeyEvent(sc_LeftArrow,false);CHECK(HandleHighScoreEntry(2)==0);
    IN_KeyEvent(0x2e,true);IN_KeyEvent(0x2e,false);CHECK(HandleHighScoreEntry(3)==0);
    IN_KeyEvent(sc_Enter,true);IN_KeyEvent(sc_Enter,false);CHECK(HandleHighScoreEntry(4)==1);
    CHECK(!strcmp(HighScores_Table()[0].name,"acb") && US_RndIndex()==rng);
    CHECK(HighScores_Save() && HighScores_Load(directory));
    CHECK(!strcmp(HighScores_Table()[0].name,"acb"));
    CHECK(BeginHighScoreEntry(1,0));IN_KeyEvent(0x1e,true);IN_KeyEvent(0x1e,false);CHECK(HandleHighScoreEntry(1)==0);
    IN_KeyEvent(sc_Escape,true);IN_KeyEvent(sc_Escape,false);CHECK(HandleHighScoreEntry(2)==1);
    CHECK(!strcmp(HighScores_Table()[1].name,"id software-'92")); /* Escape retains original buffer. */
    char file[4096];(void)snprintf(file,sizeof(file),"%s/highscores.w3h",directory);
    FILE *f=fopen(file,"r+b");CHECK(f);CHECK(fputc('X',f)!=EOF && fclose(f)==0);
    CHECK(!HighScores_Load(directory) && !HighScores_Save());
    f=fopen(file,"rb");CHECK(f);CHECK(fgetc(f)=='X' && fclose(f)==0);CHECK(remove(file)==0);
    CHECK(HighScores_Load(directory));CHECK(HighScores_Insert(123456,9,5)==0);
    CHECK(BeginHighScoreEntry(0,0));
    IN_KeyEvent(0x1e,true);IN_KeyEvent(0x1e,false);IN_KeyEvent(0x2e,true);IN_KeyEvent(0x2e,false);IN_KeyEvent(0x30,true);IN_KeyEvent(0x30,false);
    CHECK(HandleHighScoreEntry(1)==0);IN_KeyEvent(sc_Enter,true);IN_KeyEvent(sc_Enter,false);CHECK(HandleHighScoreEntry(2)==1);
    CHECK(HighScores_Save());
    puts("DOS score qualification/ties, names, editing, escape, persistence, corrupt-file protection and RNG isolation passed.");
}
int main(int argc,char **argv)
{
    CHECK(argc==4);MM_Startup();CHECK(CA_Startup(argv[1]));CHECK(VL_Startup());CHECK(VL_SetPalette(gamepal,768));Display_Connect(VL_DisplayGet,VL_DisplaySet);
    if(!strcmp(argv[3],"interface")) Interface();
    else if(!strcmp(argv[3],"hidden")) Hidden();
    else if(!strcmp(argv[3],"scores") || !strcmp(argv[3],"scores-read")) Scores(argv[2],!strcmp(argv[3],"scores-read"));
    else if(!strcmp(argv[3],"keys")) Keys(argv[2]);
    else { CHECK(Save_SetDirectory(argv[2]));if(!strcmp(argv[3],"view")) Views();else if(!strcmp(argv[3],"missing") || !strcmp(argv[3],"corrupt")) Missing();else Demos(); }
    GameStop();Display_Connect(NULL,NULL);VL_Shutdown();CA_Shutdown();MM_Shutdown();CHECK(!MM_TotalBytes());return 0;
}
