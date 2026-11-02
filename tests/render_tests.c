/* SPDX-License-Identifier: GPL-2.0-only */
#include "engine/WL_RENDER.h"
#include "engine/WL_SAVE.h"
#include "engine/WL_MAIN.h"
#include "engine/WL_DISPLAY.h"
#include "engine/ID_CA.h"
#include "engine/ID_VL.h"
#include "engine/ID_PM.h"
#include "engine/ID_US_1.h"
#include "platform/sdl_video.h"
#include "platform/sdl_audio.h"
#include "engine/ID_SD.h"
#include "platform/config_file.h"
#include <SDL3/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#define CHECK(x) do { if(!(x)) { fprintf(stderr,"%d: %s (%s; %s)\n",__LINE__,#x,CA_Error(),SDL_GetError());exit(1); } } while(0)
static uint32_t Hash(const void *data,size_t n) { const unsigned char *p=data;uint32_t h=2166136261U;while(n--) h=(h^*p++)*16777619U;return h; }
static void Pump(void) { CHECK(!VL_PollQuit());CHECK(VW_UpdateScreen()); }
/* Force long frames without stalling any individual rendering batch. The
   real dummy device drains concurrently, so servicing only once per frame
   would empty the 24-32 ms queue. */
static unsigned services,empty;
static uint64_t last_service,max_gap;
static void ServiceAudio(void)
{
    SDL_Delay(1);
    uint64_t now=SDL_GetTicksNS(),gap=now-last_service;
    if(gap>max_gap) max_gap=gap;
    last_service=now;
    if(!Audio_TestQueuedBytes()) ++empty;
    CHECK(Audio_TestQueuedBytes()>=0);Audio_Pump();++services;
}
static void AudioStress(void)
{
    CHECK(Audio_Startup());SD_StartMusic(7);Audio_Pump();
    uint8_t *pixels=malloc(1920*1080);int32_t depth[1920];wl_render_target t;
    CHECK(pixels && WL_TargetInit(&t,pixels,1920*1080,1920,1080,1920));
    CHECK(NewViewSize(19));last_service=SDL_GetTicksNS();uint64_t start=last_service;
    for(unsigned i=0;i<3;++i) CHECK(WL_RenderFrame(&t,depth,1920,true,ServiceAudio));
    CHECK(services>300 && SDL_GetTicksNS()-start>32000000);
    CHECK(!empty); /* No drained queue despite frames longer than its capacity. */
    sd_status status;SD_GetStatus(&status);CHECK(status.frames>48000/10);
    printf("Audio render stress: %u services, %u empty queues, longest gap %.2f ms.\n",services,empty,(double)max_gap/1000000);
    free(pixels);Audio_Shutdown();CHECK(NewViewSize(15));
}
static void Visual(void)
{
    CHECK(GameStart(0));InitObjects();CHECK(NewViewSize(15));
    player.x=2*TILEGLOBAL+32768;player.y=2*TILEGLOBAL+32768;player.angle=0;
    uint8_t *pixels=malloc(640*480);CHECK(pixels);int32_t depth[640];wl_render_target t;
    CHECK(WL_TargetInit(&t,pixels,640*480,640,480,640));
    unsigned char *weapon=(unsigned char *)PM_GetPage(PMSpriteStart+SPR_PISTOLREADY,NULL);
    unsigned char original[14];memcpy(original,weapon,14);weapon[6]=weapon[7]=0;
    unsigned char *near=(unsigned char *)PM_GetPage(PMSpriteStart+SPR_STAT_0,NULL);
    unsigned char *far=(unsigned char *)PM_GetPage(PMSpriteStart+SPR_STAT_1,NULL);
    memset(near+14,10,32);memset(far+14,20,32);
    CHECK(WL_RenderFrame(&t,depth,640,true,NULL));uint8_t background=pixels[193*640+319];
    statcount=2;statobjlist[0]=(statobj_t){4,2,SPR_STAT_0,dressing,false};
    statobjlist[1]=(statobj_t){5,2,SPR_STAT_1,dressing,false};
    CHECK(WL_RenderFrame(&t,depth,640,true,NULL));CHECK(pixels[193*640+319]==10);
    /* A transparent neighbor remains background, while index zero is opaque. */
    CHECK(pixels[193*640+300]!=10 && pixels[193*640+300]!=20);
    statobj_t tmp=statobjlist[0];statobjlist[0]=statobjlist[1];statobjlist[1]=tmp;
    CHECK(WL_RenderFrame(&t,depth,640,true,NULL));CHECK(pixels[193*640+319]==10);
    memset(near+14,0,32);CHECK(WL_RenderFrame(&t,depth,640,true,NULL));CHECK(!pixels[193*640+319]);
    /* Wall occlusion is independent of actor-list order and screen scaling. */
    for(unsigned y=1;y<63;++y) tilemap[3][y]=1;
    CHECK(WL_RenderFrame(&t,depth,640,true,NULL));CHECK(pixels[193*640+319]!=0 && pixels[193*640+319]!=20);
    memcpy(weapon,original,14);statcount=0;CHECK(WL_RenderFrame(&t,depth,640,true,NULL));
    CHECK(pixels[300*640+317]>=100); /* original weapon art stays in the lower center */
    CHECK(background!=10 && background!=20);
    free(pixels);GameStop();
}
int main(int argc,char **argv)
{
    CHECK(argc==4);MM_Startup();CHECK(CA_Startup(argv[1]));CHECK(VL_SetPalette(gamepal,768));CHECK(VL_Startup());
    Display_Connect(VL_DisplayGet,VL_DisplaySet);CHECK(Save_SetDirectory(argv[2]));CHECK(DemoLoopStartup(0,true));CHECK(DemoLoopStartGame(0));
    CHECK(GameStep(14));VL_FadeStep(20);CHECK(Save_Write(0,"render before",true));
    unsigned rng=US_RndIndex();uint32_t frame=Hash(framebuffer,VL_PIXELS),clock=gamestate.TimeCount;
    objtype saved_player=player,saved_actors[MAXACTORS];memcpy(saved_actors,objlist,sizeof(saved_actors));
    bool saved_vis[64][64];memcpy(saved_vis,spotvis,sizeof(saved_vis));
    static const unsigned sizes[][2]={{640,480},{1280,720},{1920,1080}};
    for(unsigned n=0;n<3;++n) {
        unsigned w=sizes[n][0],h=sizes[n][1],stride=w+8;size_t count=(size_t)stride*h;
        uint8_t *pixels=malloc(count+32);int32_t depth[1920];CHECK(pixels);memset(pixels,0xac,count+32);
        wl_render_target target;CHECK(WL_TargetInit(&target,pixels,count,w,h,stride));
        CHECK(!WL_RenderFrame(&target,depth,w-1,true,NULL));
        for(unsigned view=4;view<=19;++view) {
            CHECK(NewViewSize(view));CHECK(GameDrawPaused());
            uint32_t before=Hash(framebuffer,VL_PIXELS);
            CHECK(WL_RenderFrame(&target,depth,1920,true,NULL));
            CHECK(Hash(framebuffer,VL_PIXELS)==before && US_RndIndex()==rng && gamestate.TimeCount==clock);
            for(unsigned y=0;y<h;++y) for(unsigned x=w;x<stride;++x) CHECK(pixels[(size_t)y*stride+x]==0xac);
            for(unsigned i=0;i<32;++i) CHECK(pixels[count+i]==0xac);
            unsigned left,cw;WL_RenderContent(w,h,&left,&cw);
            CHECK(cw*3==h*4);
            /* Original HUD must be exactly aspect-scaled, including index zero. */
            for(unsigned y=h*4/5;y<h;++y) for(unsigned x=0;x<cw;++x)
                CHECK(pixels[(size_t)y*stride+left+x]==framebuffer[(size_t)(y*200/h)*320+x*320/cw]);
            for(unsigned y=0;y<h;++y) for(unsigned x=0;x<left;++x) CHECK(!pixels[(size_t)y*stride+x] && !pixels[(size_t)y*stride+w-1-x]);
            /* A second pure render produces the same image, with no allocation. */
            uint32_t image=Hash(pixels,count);CHECK(WL_RenderFrame(&target,depth,1920,true,NULL));CHECK(Hash(pixels,count)==image);
        }
        CHECK(NewViewSize(15));CHECK(GameDrawPaused());
        CHECK(WL_RenderFrame(&target,depth,1920,true,NULL));
        uint32_t native=Hash(pixels,count);CHECK(WL_RenderFrame(&target,depth,1920,false,NULL));
        CHECK(Hash(pixels,count)!=native); /* Genuine geometry differs from enlarged DOS pixels. */
        free(pixels);
        display_settings d;CHECK(VL_DisplayGet(&d));d.high_resolution=true;d.render_width=w;d.render_height=h;CHECK(VL_DisplaySet(&d));Pump();
        CHECK(VL_DisplayGet(&d) && d.high_resolution && d.render_width==w && d.render_height==h);
        uint64_t uploads=VL_TextureUploads();Pump();CHECK(VL_TextureUploads()==uploads);
        CHECK(Config_Save(argv[2]));d.high_resolution=false;CHECK(VL_DisplaySet(&d));Pump();
        CHECK(Config_Load(argv[2])==CONFIG_LOADED);Pump();CHECK(VL_DisplayGet(&d) && d.high_resolution && d.render_width==w);
        /* Recreate SDL completely to verify actual launch-time restoration. */
        VL_Shutdown();CHECK(VL_Startup());CHECK(Config_Load(argv[2])==CONFIG_LOADED);Pump();
        CHECK(VL_DisplayGet(&d) && d.high_resolution && d.render_width==w);
        int count_windows;SDL_Window **windows=SDL_GetWindows(&count_windows);CHECK(windows && count_windows==1);
        SDL_Window *window=windows[0];SDL_free(windows);
        const int windowsizes[][2]={{640,480},{1920,1080},{600,1000},{160,120},{1,1},{960,720}};
        for(unsigned i=0;i<6;++i) {
            CHECK(SDL_SetWindowSize(window,windowsizes[i][0],windowsizes[i][1]) && SDL_SyncWindow(window));Pump();
            int pw,ph;CHECK(SDL_GetWindowSizeInPixels(window,&pw,&ph));CHECK(VL_DisplayGet(&d));
            CHECK(d.pixel_width==pw && d.pixel_height==ph);
            if(pw>=4 && ph>=3) { SDL_FRect rect;CHECK(SDL_GetRenderLogicalPresentationRect(SDL_GetRenderer(window),&rect));CHECK(fabs(rect.w*3-rect.h*4)<=4); }
        }
        d.fullscreen=true;CHECK(VL_DisplaySet(&d) && SDL_SyncWindow(window));Pump();
        CHECK(VL_DisplayGet(&d) && d.fullscreen);d.fullscreen=false;CHECK(VL_DisplaySet(&d) && SDL_SyncWindow(window));Pump();
    }
    CHECK(!memcmp(&player,&saved_player,sizeof(player)) && !memcmp(saved_actors,objlist,sizeof(saved_actors)));
    CHECK(!memcmp(saved_vis,spotvis,sizeof(saved_vis)) && Hash(framebuffer,VL_PIXELS)==frame);
    CHECK(Save_Write(1,"render after",true));
    /* Failed allocations/resolutions/fullscreen requests retain the old surface. */
    display_settings d,old;CHECK(VL_DisplayGet(&old));d=old;d.render_width=1234;CHECK(!VL_DisplaySet(&d));
    d=old;d.render_width=640;d.render_height=480;VL_TestDisplayFailure(5);CHECK(!VL_DisplaySet(&d));Pump();
    CHECK(VL_DisplayGet(&d) && d.render_width==old.render_width && d.high_resolution);
    d=old;d.render_width=640;d.render_height=480;VL_TestDisplayFailure(6);CHECK(!VL_DisplaySet(&d));Pump();
    d=old;d.high_resolution=false;d.fullscreen=true;VL_TestDisplayFailure(1);CHECK(!VL_DisplaySet(&d));Pump();
    CHECK(VL_DisplayGet(&d) && d.high_resolution);
    /* Switching display preference cannot consume a simulation tic or RNG. */
    for(unsigned n=0;n<30;++n) { d.high_resolution=!d.high_resolution;CHECK(VL_DisplaySet(&d));Pump();CHECK(US_RndIndex()==rng && gamestate.TimeCount==clock); }
    if(!strcmp(argv[3],"synthetic")) Visual();
    else AudioStress();
    GameStop();Display_Connect(NULL,NULL);VL_Shutdown();CA_Shutdown();MM_Shutdown();CHECK(!MM_TotalBytes());
    puts("Native render targets, all sizes/views, HUD, isolation, persistence, switching and failure recovery passed.");return 0;
}
