/* SPDX-License-Identifier: GPL-2.0-only */
#include "WL_FRAME_CLOCK.h"
void WL_FrameClockReset(wl_frame_clock *clock)
{
    if(clock) { clock->previous_ns=0;clock->current_ns=0;clock->ready=false; }
}
void WL_FrameClockObserve(wl_frame_clock *clock,uint64_t tick_ns)
{
    if(!clock) return;
    if(!clock->ready || tick_ns<=clock->current_ns) {
        clock->previous_ns=tick_ns;clock->current_ns=tick_ns;clock->ready=true;
        return;
    }
    clock->previous_ns=clock->current_ns;clock->current_ns=tick_ns;
}
double WL_FrameClockAlpha(const wl_frame_clock *clock,uint64_t now_ns)
{
    if(!clock || !clock->ready || clock->current_ns<=clock->previous_ns) return 0.0;
    /* Interpolate from previous authoritative state to current state with
       one tick of presentation latency, avoiding future-state prediction. */
    if(now_ns<=clock->current_ns) return 0.0;
    uint64_t span=clock->current_ns-clock->previous_ns;
    uint64_t elapsed=now_ns-clock->current_ns;
    if(elapsed>=span) return 1.0;
    return (double)elapsed/(double)span;
}
