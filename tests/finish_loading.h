/* SPDX-License-Identifier: GPL-2.0-only */
/* Existing input scenarios use short, fixed clocks. Finish the new presentation
   before those scenarios resume; loading_tests.c checks its real clock/events. */
#include "engine/WL_DEF.h"
#include "engine/WL_SAVE.h"
static inline void TestFinishLoading(uint64_t ticks)
{
    uint64_t loading_tick=ticks;
    while(GameLoading() && loading_tick<ticks+1000) CHECK(StepPreloadGraphics(++loading_tick));
    CHECK(!GameLoading());GamePauseClock(ticks);
}
