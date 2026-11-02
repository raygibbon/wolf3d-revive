/* SPDX-License-Identifier: GPL-2.0-only */
/* Capture the untouched indexed DOS framebuffer at every supported view size. */
#include "engine/WL_DEF.h"
#include "engine/WL_SAVE.h"
#include "engine/ID_CA.h"
#include "engine/ID_VL.h"
#include <stdio.h>
static unsigned Hash(void) { unsigned h=2166136261U;for(unsigned i=0;i<VL_PIXELS;++i) h=(h^framebuffer[i])*16777619U;return h; }
int main(int argc,char **argv)
{
    if(argc!=2) return 2;
    MM_Startup();if(!CA_Startup(argv[1])) return 2;
    for(unsigned size=4;size<=19;++size) {
        if(!NewViewSize(size) || !GameStart(0)) return 2;
        printf("%u,0,%u\n",size,Hash());
        for(unsigned t=1;t<=60;++t) if(!GameStep(t)) return 2;
        printf("%u,60,%u\n",size,Hash());GameStop();
    }
    CA_Shutdown();MM_Shutdown();return MM_TotalBytes()?2:0;
}
