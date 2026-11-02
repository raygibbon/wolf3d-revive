/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef WL_RENDER_H
#define WL_RENDER_H
#include "WL_RENDER_TARGET.h"
#include "WL_DEF.h"
#include "WL_ASPECT.h"
/* Presentation only. Never changes spotvis, actors, inventory or RNG. */
bool WL_RenderRay(int angle,rayhit *hit);
unsigned WL_CeilingColor(void);
bool WL_RenderResolution(unsigned width,unsigned height);
/* Centered 4:3 image within the selected output canvas. */
void WL_RenderContent(unsigned width,unsigned height,unsigned *x,unsigned *w);
/* Optional main-thread service runs between bounded rendering batches (audio).
   NULL keeps the pass entirely read-only. Caller supplies persistent target and per-column depth storage. */
bool WL_RenderFrameAspect(wl_render_target *target,int32_t *depth,size_t columns,bool world,
                          unsigned aspect_width,unsigned aspect_height,void (*service)(void));
bool WL_RenderFrame(wl_render_target *target,int32_t *depth,size_t columns,bool world,void (*service)(void));
#endif
