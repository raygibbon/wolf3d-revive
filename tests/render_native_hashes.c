/* SPDX-License-Identifier: GPL-2.0-only */
/* Pixel oracle captured before the Milestone 9 software-loop optimisations. */
#include "engine/WL_RENDER.h"
#include "engine/WL_SAVE.h"
#include "engine/ID_CA.h"
#include <stdio.h>
#include <stdlib.h>
static unsigned Hash(const unsigned char *p,size_t n)
{ unsigned h=2166136261U;while(n--) h=(h^*p++)*16777619U;return h; }
int main(int argc,char **argv)
{
    if(argc!=2) return 2;
    MM_Startup();if(!CA_Startup(argv[1])) return 2;
    const unsigned sizes[][2]={{640,480},{1280,720},{1920,1080},{1920,803},{1920,540}};
    const unsigned aspects[][2]={{4,3},{16,9},{16,10},{21,9},{32,9}};
    unsigned char *pixels=malloc(1920*1080);int32_t depth[1920];
    if(!pixels) return 2;
    for(unsigned map=0;map<60;++map) {
        if(!GameStartMap(0,map,2,0)) return 2;
        for(unsigned n=0;n<5;++n) for(unsigned a=0;a<5;++a) {
            if(!NewViewSize(n%2?4:19)) return 2;
            player.angle=(int)((map*37+a*71)%360);
            if(!GameDrawPaused()) return 2;
            wl_render_target t;
            if(!WL_TargetInit(&t,pixels,1920*1080,sizes[n][0],sizes[n][1],sizes[n][0]) ||
               !WL_RenderFrameAspect(&t,depth,1920,true,aspects[a][0],aspects[a][1],NULL)) return 2;
            printf("%u,%u,%u,%u\n",map,n,a,Hash(pixels,(size_t)t.width*t.height));
        }
    }
    free(pixels);GameStop();CA_Shutdown();MM_Shutdown();return MM_TotalBytes()?2:0;
}
