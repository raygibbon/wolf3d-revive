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
    WL_FrameClockObserve(&c,150); /* Clock regression resets presentation. */
    assert(WL_FrameClockAlpha(&c,180)==0.0);
    WL_FrameClockReset(&c);
    assert(WL_FrameClockAlpha(&c,1000)==0.0);
    return 0;
}
