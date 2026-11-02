/* SPDX-License-Identifier: GPL-2.0-only */
#include "WL_FRAME_CLOCK.h"
#include "ID_TIME.h"
void WL_FrameClockReset(wl_frame_clock *clock)
{
    if(clock) { clock->previous_ns=0;clock->current_ns=0;clock->ready=false; }
}
void WL_FrameClockObserve(wl_frame_clock *clock,uint64_t tick_ns)
{
    if(!clock) return;
    if(!clock->ready || tick_ns<clock->current_ns) {
        clock->previous_ns=tick_ns;clock->current_ns=tick_ns;clock->ready=true;
        return;
    }
    if(tick_ns==clock->current_ns) return;
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

uint64_t WL_TicksFromNS(uint64_t ns)
{ return TimeCount_FromNS(ns); }
uint64_t WL_TickNS(uint64_t tick)
{
    if(tick/70>UINT64_MAX/1000000000U) return UINT64_MAX;
    uint64_t whole=tick/70*1000000000U,part=(tick%70*1000000000U+69)/70;
    return whole>UINT64_MAX-part?UINT64_MAX:whole+part;
}
bool WL_TickNext(uint64_t *completed,uint64_t now_ns)
{
    if(!completed || *completed>=WL_TicksFromNS(now_ns)) return false;
    ++*completed;return true;
}
uint64_t WL_FrameDeadline(uint64_t now_ns,unsigned fps)
{
    if(fps<1 || fps>1000) return now_ns;
    uint64_t frame=now_ns/1000000000U*fps+(now_ns%1000000000U)*fps/1000000000U+1;
    if(frame/fps>UINT64_MAX/1000000000U) return UINT64_MAX;
    uint64_t whole=frame/fps*1000000000U,part=(frame%fps*1000000000U+fps-1)/fps;
    return whole>UINT64_MAX-part?UINT64_MAX:whole+part;
}
