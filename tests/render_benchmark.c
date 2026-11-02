/* SPDX-License-Identifier: GPL-2.0-only */
/* Optional owned-data microbenchmark: scene only, no SDL upload or simulation. */
#include "engine/WL_RENDER.h"
#include "engine/WL_SAVE.h"
#include "engine/ID_CA.h"
#include <SDL3/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#define CHECK(x) do { if(!(x)) { fprintf(stderr,"Benchmark failed: %s (%s)\n",#x,CA_Error());exit(1); } } while(0)
int main(int argc,char **argv)
{
    CHECK(argc==2);MM_Startup();CHECK(CA_Startup(argv[1]));
    CHECK(GameStart(0) && NewViewSize(19) && GameDrawPaused());
    static const unsigned sizes[][2]={{640,480},{1280,720},{1920,1080},{3440,1440},{3840,1080},{4096,2160}};
    for(unsigned n=0;n<sizeof(sizes)/sizeof(sizes[0]);++n) {
        unsigned w=sizes[n][0],h=sizes[n][1];size_t bytes=(size_t)w*h;
        uint8_t *pixels=malloc(bytes);int32_t depth[WL_TARGET_MAX_WIDTH];wl_render_target target;
        CHECK(pixels && WL_TargetInit(&target,pixels,bytes,w,h,w));
        uint64_t start=SDL_GetTicksNS();
        for(unsigned i=0;i<20;++i) CHECK(WL_RenderFrameAspect(&target,depth,WL_TARGET_MAX_WIDTH,true,w,h,NULL));
        printf("%ux%u %.3f ms/scene; indexed+RGBA storage budget %zu bytes\n",w,h,(double)(SDL_GetTicksNS()-start)/20000000,bytes*5);
        free(pixels);
    }
    GameStop();CA_Shutdown();MM_Shutdown();CHECK(!MM_TotalBytes());return 0;
}
