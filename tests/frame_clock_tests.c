/* SPDX-License-Identifier: GPL-2.0-only */
#include "engine/WL_FRAME_CLOCK.h"
#include <assert.h>
int main(void)
{
    wl_frame_clock c={0};
    assert(WL_FrameClockAlpha(&c,100)==0.0);
    WL_FrameClockObserve(&c,100);
    assert(WL_FrameClockAlpha(&c,110)==0.0);
    WL_FrameClockObserve(&c,200);
    assert(WL_FrameClockAlpha(&c,200)==0.0);
    assert(WL_FrameClockAlpha(&c,250)==0.5);
    assert(WL_FrameClockAlpha(&c,400)==1.0);
    WL_FrameClockObserve(&c,200);assert(WL_FrameClockAlpha(&c,250)==0.5);
    WL_FrameClockObserve(&c,150); /* Clock regression resets presentation. */
    assert(WL_FrameClockAlpha(&c,180)==0.0);
    WL_FrameClockReset(&c);
    assert(WL_FrameClockAlpha(&c,1000)==0.0);
    const unsigned rates[]={60,120,144,165,240,1000};
    for(unsigned n=0;n<6;++n) {
        uint64_t tick=0,now=0;
        for(unsigned frame=0;frame<rates[n]*10;++frame) {
            now=WL_FrameDeadline(now,rates[n]);
            unsigned budget=0;while(budget<8 && WL_TickNext(&tick,now)) ++budget;
        }
        assert(now==10000000000ULL && tick==700);
    }
    uint64_t tick=0;for(unsigned i=0;i<8;++i) assert(WL_TickNext(&tick,1000000000));
    assert(tick==8);while(WL_TickNext(&tick,1000000000)) {} assert(tick==70);
    assert(WL_TickNS(70)==1000000000 && WL_TicksFromNS(WL_TickNS(1))==1);
    assert(!WL_TickNext(0,1000));assert(WL_TickNS(UINT64_MAX)==UINT64_MAX);
    assert(WL_FrameDeadline(UINT64_MAX,240)==UINT64_MAX);
    assert(WL_FrameDeadline(100,0)==100 && WL_FrameDeadline(100,1001)==100);
    WL_FrameClockObserve(0,1);WL_FrameClockReset(0);
    return 0;
}
