/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef WL_FRAME_CLOCK_H
#define WL_FRAME_CLOCK_H
#include <stdbool.h>
#include <stdint.h>
/* Presentation-only timing utility. No gameplay clock or RNG is modified.
   Call after authoritative simulation stepping, never instead of it. */
typedef struct { uint64_t previous_ns,current_ns; bool ready; } wl_frame_clock;
void WL_FrameClockReset(wl_frame_clock *clock);
void WL_FrameClockObserve(wl_frame_clock *clock,uint64_t tick_ns);
/* 0..1 interpolation fraction, clamped against zero/invalid intervals. */
double WL_FrameClockAlpha(const wl_frame_clock *clock,uint64_t now_ns);
/* Rational deadlines; fixed simulation debt is consumed one tick at a time.
   Callers bound catch-up work without dropping debt. */
uint64_t WL_TicksFromNS(uint64_t ns);
uint64_t WL_TickNS(uint64_t tick);
bool WL_TickNext(uint64_t *completed,uint64_t now_ns);
uint64_t WL_FrameDeadline(uint64_t now_ns,unsigned fps);
#endif
