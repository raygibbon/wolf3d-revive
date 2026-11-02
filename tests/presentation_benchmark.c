/* SPDX-License-Identifier: GPL-2.0-only */
/* Full SDL path, owned E1M1, real elapsed time. Target override belongs only
   to this diagnostic harness; it is never a player setting. */
#include "engine/WL_PROFILE.h"
#include "engine/WL_MAIN.h"
#include "engine/WL_SAVE.h"
#include "engine/WL_RENDER.h"
#include "engine/WL_CONTROLS.h"
#include "engine/ID_CA.h"
#include "engine/ID_VL.h"
#include "engine/ID_SD.h"
#include "platform/sdl_video.h"
#include "platform/sdl_audio.h"
#include <SDL3/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdatomic.h>
#include <string.h>
#define CHECK(x) do { if(!(x)) { fprintf(stderr,"%s: %s / %s\n",#x,CA_Error(),SDL_GetError());exit(1); } } while(0)
wl_profile WL_Profile;
uint64_t WL_ProfileNow(void) { return SDL_GetTicksNS(); }
#ifdef WOLF3D_ALLOC_PROFILE
static atomic_ulong allocations;
void *__real_malloc(size_t n);
void *__real_calloc(size_t n,size_t s);
void *__real_realloc(void *p,size_t n);
void *__wrap_malloc(size_t n) { ++allocations;return __real_malloc(n); }
void *__wrap_calloc(size_t n,size_t s) { ++allocations;return __real_calloc(n,s); }
void *__wrap_realloc(void *p,size_t n) { ++allocations;return __real_realloc(p,n); }
#else
static atomic_ulong allocations;
#endif
static int Compare(const void *a,const void *b)
{ double x=*(const double *)a,y=*(const double *)b;return (x>y)-(x<y); }
int main(int argc,char **argv)
{
    CHECK(argc==2 || argc==3);
    if(argc==3) { char *end;double rate=strtod(argv[2],&end);CHECK(!*end && rate>=1 && rate<=240); }
    MM_Startup();CHECK(CA_Startup(argv[1]));
    CHECK(VL_SetPalette(gamepal,768) && VL_Startup() && Audio_Startup());
    CHECK(DemoLoopStartup(VL_GetTicks70(),true));
    const unsigned sizes[][2]={{640,480},{1280,720},{1920,1080}};
    int window_count;SDL_Window **windows=SDL_GetWindows(&window_count);CHECK(windows && window_count==1);
    SDL_Renderer *renderer=SDL_GetRenderer(windows[0]);SDL_free(windows);
    printf("video=%s, renderer=%s, audio=%s; owned E1M1, view 19, 4 seconds/canvas\n",
           SDL_GetCurrentVideoDriver(),SDL_GetRendererName(renderer),SDL_GetCurrentAudioDriver());
    for(unsigned n=0;n<3;++n) {
        CHECK(DemoLoopStartGame(VL_GetTicks70()) && NewViewSize(19));
        display_settings d;CHECK(VL_DisplayGet(&d));d.high_resolution=d.smooth=true;
        d.render_width=sizes[n][0];d.render_height=sizes[n][1];d.aspect=2;d.scale=0;d.width=1280;d.height=720;
        d.vsync=SDL_getenv("M8_VSYNC")!=NULL;CHECK(VL_DisplaySet(&d));
        /* Warm page caches, audio and texture allocation before measuring. */
        uint64_t warm_start=SDL_GetTicksNS();
        for(unsigned i=0;i<20 || SDL_GetTicksNS()-warm_start<300000000;++i) { CHECK(DemoLoop(VL_GetTicks70()) && VW_UpdateScreen());Audio_Pump(); }
        uint64_t tick=VL_GetTicks70(),initial_tick=tick,start=SDL_GetTicksNS(),last=start,next=0;
        GamePauseClock(tick);uint32_t initial_clock=gamestate.TimeCount;
        double intervals[4096];unsigned count=0;clock_t cpu=clock();unsigned long initial_allocations=atomic_load(&allocations);
        memset(&WL_Profile,0,sizeof(WL_Profile));
        while(SDL_GetTicksNS()-start<4000000000ULL && count<4096) {
            CHECK(!VL_PollQuit());uint64_t now=SDL_GetTicksNS();
            for(unsigned steps=0;steps<8 && WL_TickNext(&tick,now);++steps) CHECK(DemoLoop(tick));
            Audio_Pump();now=SDL_GetTicksNS();
            double refresh=argc==3?strtod(argv[2],NULL):VL_MonitorRefreshRate();
            if(now>=next) {
                uint64_t begin=now;CHECK(VW_UpdateScreen());now=SDL_GetTicksNS();
                intervals[count++]=(double)(now-last)/1000000.0;last=now;
                next=WL_PresentationDeadline(begin,now,refresh,VL_VSyncPaced(refresh));
            }
            now=SDL_GetTicksNS();uint64_t wake=WL_TickNS(tick+1);if(next<wake) wake=next;
            if(wake>now) SDL_DelayNS(wake-now>4000000?4000000:wake-now);
        }
        uint64_t finish=SDL_GetTicksNS();
        double seconds=(double)(finish-start)/1000000000.0;
        double cpu_percent=100.0*(double)(clock()-cpu)/CLOCKS_PER_SEC/seconds;
        qsort(intervals,count,sizeof(*intervals),Compare);
        double sum=0,squares=0;for(unsigned i=0;i<count;++i) { sum+=intervals[i];squares+=intervals[i]*intervals[i]; }
        printf("%ux%u target=%.3f actual=%.2f FPS mean=%.3f p95=%.3f p99=%.3f variance=%.4f ms^2 cpu=%.1f%% ticks=%llu/%llu game=%u allocations=%lu\n",
               sizes[n][0],sizes[n][1],argc==3?strtod(argv[2],NULL):VL_MonitorRefreshRate(),count/seconds,sum/count,
               intervals[(count-1)*95/100],intervals[(count-1)*99/100],squares/count-(sum/count)*(sum/count),cpu_percent,
               (unsigned long long)(tick-initial_tick),(unsigned long long)(WL_TicksFromNS(finish)-initial_tick),gamestate.TimeCount-initial_clock,atomic_load(&allocations)-initial_allocations);
        printf("mean ms/present: rays+walls=%.4f sprites=%.4f capture=%.4f sample=%.4f convert=%.4f upload=%.4f present=%.4f empty_queue_observations=%llu; surface=%zu bytes\n",
               (double)WL_Profile.rays/count/1000000.0,(double)WL_Profile.sprites/count/1000000.0,
               (double)WL_Profile.capture/count/1000000.0,(double)WL_Profile.sample/count/1000000.0,
               (double)WL_Profile.convert/count/1000000.0,(double)WL_Profile.upload/count/1000000.0,
               (double)WL_Profile.present/count/1000000.0,(unsigned long long)WL_Profile.empty_queues,(size_t)sizes[n][0]*sizes[n][1]*5);
        CHECK(gamestate.TimeCount-initial_clock==tick-initial_tick);
        const char *captures=SDL_getenv("M8_CAPTURE");
        if(captures) { char path[4096];CHECK(snprintf(path,sizeof(path),"%s/m8-%ux%u.bmp",captures,sizes[n][0],sizes[n][1])>0);CHECK(VL_Capture(path)); }
    }
    GameStop();Audio_Shutdown();VL_Shutdown();CA_Shutdown();MM_Shutdown();return 0;
}
