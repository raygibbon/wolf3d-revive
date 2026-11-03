/* SPDX-License-Identifier: GPL-2.0-only */
#include "engine/WL_DEF.h"
#include "engine/WL_MAIN.h"
#include "engine/WL_MENU.h"
#include "engine/WL_SAVE.h"
#include "engine/WL_DISPLAY.h"
#include "engine/WL_PRESENT.h"
#include "engine/ID_CA.h"
#include "engine/ID_PM.h"
#include "engine/ID_IN.h"
#include "engine/ID_VH.h"
#include "engine/ID_US_1.h"
#include "engine/ID_SD.h"
#include "platform/sdl_video.h"
#include "platform/sdl_audio.h"
#include <SDL3/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(x) do { if(!(x)) { fprintf(stderr,"%d: %s (%s; %s)\n",__LINE__,#x,CA_Error(),SDL_GetError());exit(1); } } while(0)
static uint64_t ticks;
static SDL_Window *window;
static SDL_Renderer *renderer;
static unsigned Uncached(void)
{
    unsigned n=0;for(unsigned p=0;p<ChunksInFile;++p) if(PM_PagePresent(p) && !PM_PageCached(p)) ++n;return n;
}
static void Pump(void) { CHECK(!VL_PollQuit());CHECK(DemoLoop(++ticks));Audio_Pump();CHECK(VW_UpdateScreen()); }
static void Geometry(int w,int h)
{
    int aw,ah;CHECK(SDL_GetWindowSize(window,&aw,&ah));CHECK(aw==w && ah==h);
    if(!GameLoading()) return;
    SDL_FRect rect;CHECK(SDL_GetRenderLogicalPresentationRect(renderer,&rect));
    CHECK(rect.w*3-rect.h*4>=-4 && rect.w*3-rect.h*4<=4);
    CHECK(rect.x>=0 && rect.y>=0);
    CHECK(rect.x*2+rect.w-(float)w>=-2 && rect.x*2+rect.w-(float)w<=2);
    CHECK(rect.y*2+rect.h-(float)h>=-2 && rect.y*2+rect.h-(float)h<=2);
}
static void FinishLoading(void)
{
    unsigned n=0;while(GameLoading() && ++n<500) Pump();CHECK(!GameLoading());
}
static void Bar(void)
{
    for(unsigned current=0;current<=5;++current) {
        memset(framebuffer,201,VL_PIXELS);PreloadUpdate(current,5);
        unsigned width=214*current/5;
        for(unsigned y=0;y<200;++y) for(unsigned x=0;x<320;++x) {
            unsigned expected=201;
            if(y>=101 && y<=102 && x>=53 && x<267) {
                unsigned column=x-53;
                expected=column<width?0x37:0;
                if(y==101 && column+1<width) expected=0x32;
            }
            CHECK(framebuffer[y*320+x]==expected);
        }
    }
}
static void Loading(bool owned)
{
    static const int sizes[][2]={{960,720},{1280,720},{1280,800},{1720,720}};
    display_settings d;CHECK(VL_DisplayGet(&d));d.high_resolution=true;d.render_width=640;d.render_height=480;d.aspect=2;d.integer=false;d.smooth=true;CHECK(VL_DisplaySet(&d));
    for(unsigned size=0;size<4;++size) {
        int w=sizes[size][0],h=sizes[size][1];CHECK(SDL_SetWindowSize(window,w,h));CHECK(SDL_SyncWindow(window));CHECK(!VL_PollQuit());
        CHECK(DemoLoopStartGame(ticks));CHECK(GameLoading() && !GameCanSave());
        unsigned total=Uncached(),rng=US_RndIndex();uint32_t time=gamestate.TimeCount;
        w3r_byte expected[VL_PIXELS];memcpy(expected,framebuffer,sizeof(expected));
        memset(expected,127,320*160);
        CHECK(CA_CacheGrChunk(GETPSYCHEDPIC));
        unsigned pw=pictable[GETPSYCHEDPIC-STARTPICS].width,ph=pictable[GETPSYCHEDPIC-STARTPICS].height;
        if(owned) CHECK(pw==224 && ph==48);
        for(unsigned plane=0;plane<4;++plane) for(unsigned y=0;y<ph;++y) for(unsigned x=0;x<pw/4;++x)
            expected[(56+y)*320+48+4*x+plane]=((const w3r_byte *)grsegs[GETPSYCHEDPIC])[plane*pw/4*ph+y*pw/4+x];
        CHECK(!memcmp(expected,framebuffer,sizeof(expected)));
        for(unsigned i=0;i<768;++i) CHECK(!palette[i]);
        CHECK(VW_UpdateScreen());Geometry(w,h);
        for(unsigned n=0;n<29;++n) {
            Pump();
            for(unsigned i=0;i<768;++i) CHECK(palette[i]==(unsigned)gamepal[i]*(n+1)/30);
            if(n==10) { IN_KeyEvent(sc_Space,true);IN_KeyEvent(sc_Space,false); }
        }
        CHECK(Uncached()==total);
        Pump();CHECK(!memcmp(palette,gamepal,768));
        unsigned loaded=total-Uncached();CHECK(loaded==(total<32?total:32));
        if(total) {
            unsigned width=214*loaded/total;
            for(unsigned x=0;x<214;++x) {
                CHECK(framebuffer[102*320+53+x]==(x<width?0x37:0));
                CHECK(framebuffer[101*320+53+x]==(x+1<width?0x32:x<width?0x37:0));
            }
        }
        while(Uncached()) Pump();
        uint64_t done=ticks;for(unsigned n=0;n<69;++n) Pump();
        CHECK(!memcmp(palette,gamepal,768) && GameLoading());
        const char *capture=getenv("WOLF3D_LOADING_CAPTURE");
        if(capture && owned && size==0) CHECK(VL_Capture(capture));
        Pump();CHECK(ticks==done+70 && VL_Fading());
        for(unsigned n=0;n<30;++n) Pump();
        for(unsigned i=0;i<768;++i) CHECK(!palette[i]);
        CHECK(framebuffer[(VIEWY+VIEWHEIGHT)*320+VIEWX-1]==124);
        CHECK(GameLoading());for(unsigned n=0;n<30;++n) Pump();
        CHECK(!GameLoading() && !memcmp(palette,gamepal,768));
        CHECK(US_RndIndex()==rng && gamestate.TimeCount==time);Geometry(w,h);
        /* Fresh acknowledgement skips the 70-tic hold, never the page reads. */
        CHECK(StartPreloadGraphics(ticks));for(unsigned n=0;n<30;++n) Pump();
        while(Uncached()) { Pump(); }
        IN_KeyEvent(sc_Space,true);IN_KeyEvent(sc_Space,false);Pump();CHECK(VL_Fading());FinishLoading();
        CHECK(Save_Write(0,"loading restore",true));CHECK(Save_Load(0,ticks));
        CHECK(GamePreloadPending());CHECK(StartPreloadGraphics(ticks));FinishLoading();
        /* Actual GameLoop exits: ordinary floor, secret entry and return. */
        const int maps[]={0,0,9};const exit_t exits[]={ex_completed,ex_secretlevel,ex_completed};
        for(unsigned floor=0;floor<3;++floor) {
            if(!owned) break;
            CHECK(GameStartMap(ticks,(unsigned)maps[floor],2,0));GamePreloadConsumed();
            playstate=exits[floor];Pump();CHECK(GameIntermission());
            IN_KeyEvent(sc_Space,true);IN_KeyEvent(sc_Space,false);Pump();
            IN_KeyEvent(sc_Space,true);IN_KeyEvent(sc_Space,false);Pump();
            CHECK(GameLoading());CHECK(gamestate.mapon==(floor==0?1:floor==1?9:1));FinishLoading();Geometry(w,h);
        }
    }
}
static void Death(bool audio)
{
    const int starts[]={0,0,0,0,359,1,180,90,350,10};
    const int dx[]={1,-1,0,0,1,1,1,0,2,2},dy[]={0,0,-1,1,0,0,0,1,-1,1};
    const int targets[]={0,180,90,270,0,0,0,270,26,333};
    for(unsigned size=4;size<=19;++size) for(unsigned a=0;a<10;++a) {
        CHECK(NewViewSize(size));CHECK(GameStart(ticks));GamePreloadConsumed();
        player.angle=starts[a];objtype attacker={0};attacker.x=player.x+dx[a]*65536;attacker.y=player.y+dy[a]*65536;
        TakeDamage(100,&attacker);CHECK(gamestate.died && killerobj && killerobj->x==attacker.x && killerobj->y==attacker.y);
        attacker.x=attacker.y=0; /* Saved killer position survives actor reuse. */
        unsigned rng=US_RndIndex();uint32_t time=gamestate.TimeCount;
        int angle=starts[a],target=targets[a],delta=(target-angle+540)%360-180;
        unsigned tics=0;
        do {
            unsigned dt=a%2?3:1;
            for(unsigned i=0;i<dt;++i) { int turn=delta>2?2:delta< -2?-2:delta;angle=(angle+turn+360)%360;delta-=turn;++tics;if(!delta) { dt=i+1;break; } }
            ticks+=dt;CHECK(GameStep(ticks));CHECK(player.angle==angle && gamestate.weapon==-1);
            CHECK(WL_PresentCurrent() && WL_PresentCurrent()->died && WL_PresentCurrent()->weapon<=3);
            if(a==1 && tics<=3) CHECK(VW_UpdateScreen());
            if(audio && tics<=3) { sd_status sound;SD_GetStatus(&sound);CHECK(sound.sound==PLAYERDEATHSND); }
        } while(delta);
        uint32_t seed;CHECK(GameDeathPresentation(&seed)==2 && seed==1 && player.angle==target);
        if(a==1) CHECK(VW_UpdateScreen());
        w3r_byte before[VL_PIXELS];memcpy(before,framebuffer,sizeof(before));
        uint32_t expected_seed=1;
        for(unsigned p=0;p<64000/70;++p) {
            unsigned x=expected_seed>>8,y=(expected_seed-1)&255U;
            expected_seed=(expected_seed>>1)^((expected_seed&1U)?UINT32_C(0x12000):0U);
            if(x<VIEWWIDTH && y<VIEWHEIGHT) before[(VIEWY+y)*320+VIEWX+x]=4;
        }
        CHECK(GameStep(++ticks));CHECK(GameDeathPresentation(&seed)==2 && seed==expected_seed && !memcmp(before,framebuffer,sizeof(before)));
        if(a==0) { IN_KeyEvent(sc_Space,true);IN_KeyEvent(sc_Space,false); }
        unsigned n=0;while(GameDeathPresentation(NULL)==2 && ++n<150) CHECK(GameStep(++ticks));CHECK(n<150 && !IN_AnyPressed());
        for(unsigned y=0;y<VIEWHEIGHT;++y) for(unsigned x=0;x<VIEWWIDTH;++x) CHECK(framebuffer[(VIEWY+y)*320+VIEWX+x]==4);
        CHECK(US_RndIndex()==rng && gamestate.TimeCount==time);
        if(a==1) CHECK(VW_UpdateScreen());
        if(a==9) { IN_KeyEvent(sc_Space,true);IN_KeyEvent(sc_Space,false); }
        else {
            for(unsigned i=0;i<99;++i) { CHECK(GameStep(++ticks)); }
            CHECK(gamestate.lives==3);
        }
        CHECK(GameStep(++ticks));CHECK(gamestate.lives==2 && !gamestate.died && !GamePreloadPending() && !GameLoading());
    }
}
int main(int argc,char **argv)
{
    CHECK(argc==4);bool owned=!strcmp(argv[3],"owned");MM_Startup();CHECK(CA_Startup(argv[1]));CHECK(VL_Startup());
    CHECK(Save_SetDirectory(argv[2]));int count;SDL_Window **windows=SDL_GetWindows(&count);CHECK(windows && count==1);window=windows[0];SDL_free(windows);renderer=SDL_GetRenderer(window);CHECK(renderer);
    CHECK(DemoLoopStartup(0,true));if(owned) CHECK(Audio_Startup());
    Bar();Loading(owned);Death(owned);
    CHECK(DemoLoopStartGame(ticks) && GameLoading());
    SDL_Event close_event={0};close_event.type=SDL_EVENT_QUIT;CHECK(SDL_PushEvent(&close_event));CHECK(VL_PollQuit());
    GameStop();Audio_Shutdown();VL_Shutdown();CA_Shutdown();MM_Shutdown();CHECK(!MM_TotalBytes());
    puts("Get Psyched pixels, palette, actual cache progress, tics, saves/floors, SDL aspect/window invariance and all-size DOS death turns/fizzle passed.");return 0;
}
