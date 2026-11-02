/* SPDX-License-Identifier: GPL-2.0-only */
#include "engine/WL_RENDER_TARGET.h"
#include <assert.h>
#include <stdint.h>
#include <string.h>
int main(void)
{
    uint8_t pixels[4*6];memset(pixels,0xaa,sizeof(pixels));
    wl_render_target t={0};
    assert(!WL_TargetInit(&t,pixels,sizeof(pixels),0,4,6));
    assert(!WL_TargetInit(&t,pixels,sizeof(pixels),5,4,4));
    assert(!WL_TargetInit(&t,pixels,sizeof(pixels)-1,4,4,6));
    assert(!WL_TargetInit(&t,pixels,sizeof(pixels),UINT32_MAX,UINT32_MAX,UINT32_MAX));
    assert(WL_TargetInit(&t,pixels,sizeof(pixels),4,4,6));
    assert(WL_TargetClear(&t,7));
    for(unsigned y=0;y<4;++y) for(unsigned x=0;x<6;++x)
        assert(pixels[y*6+x]==(x<4?7:0xaa));
    assert(!WL_TargetPixel(&t,4,0,1));
    assert(!WL_TargetPixel(&t,0,4,1));
    assert(WL_TargetPixel(&t,3,3,9));
    assert(pixels[3*6+3]==9);
    wl_render_target valid=t;
    assert(!WL_TargetInit(&t,NULL,0,4,4,6) && !memcmp(&t,&valid,sizeof(t)));
    t.stride=1;assert(!WL_TargetClear(&t,0) && !WL_TargetPixel(&t,1,1,0));t=valid;
    t.capacity=1;assert(!WL_TargetClear(&t,0) && !WL_TargetPixel(&t,0,0,0));t=valid;
    t.height=UINT32_MAX;assert(!WL_TargetClear(&t,0) && !WL_TargetPixel(&t,0,0,0));
    assert(!WL_TargetPixel(NULL,0,0,0) && !WL_TargetClear(NULL,0));
    return 0;
}
