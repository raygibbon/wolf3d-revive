/* SPDX-License-Identifier: GPL-2.0-only */
#include "engine/WL_ASPECT.h"
#include <assert.h>
#include <math.h>
int main(void)
{
    wl_rectangle r={0};double f=0;
    assert(WL_AspectFit(1920,1080,4,3,&r));
    assert(r.width==1440 && r.height==1080 && r.x==240 && r.y==0);
    assert(WL_AspectFit(1280,720,16,9,&r));
    assert(r.width==1280 && r.height==720 && r.x==0 && r.y==0);
    assert(WL_AspectFit(800,600,16,9,&r));
    assert(r.width==800 && r.height==450 && r.y==75);
    assert(!WL_AspectFit(0,1080,4,3,&r));
    assert(!WL_AspectFit(1920,1080,0,3,&r));
    assert(WL_HorizontalFov(60,4,3,&f)&&fabs(f-60)<0.000001);
    assert(WL_HorizontalFov(60,16,9,&f)&&f>60);
    assert(WL_HorizontalFov(60,21,9,&f)&&f>70);
    assert(!WL_HorizontalFov(0,16,9,&f));
    return 0;
}
