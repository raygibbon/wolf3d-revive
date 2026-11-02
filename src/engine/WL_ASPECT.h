/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef WL_ASPECT_H
#define WL_ASPECT_H
#include <stdbool.h>
/* Pure geometry helpers for a future high-resolution renderer.
   No simulation state, renderer ownership or game-data dependencies. */
typedef struct { unsigned x,y,width,height; } wl_rectangle;
/* Preserve source display aspect in a destination canvas, using integer
   arithmetic. Width/height are physical/display pixel units. */
bool WL_AspectFit(unsigned canvas_w,unsigned canvas_h,
                  unsigned aspect_w,unsigned aspect_h,wl_rectangle *out);
/* Horizontal camera FOV for aspect ratios when vertical framing is fixed.
   Takes the horizontal FOV of a 4:3 reference in degrees. */
bool WL_HorizontalFov(double original_fov_deg,
                      unsigned aspect_w,unsigned aspect_h,double *out);
#endif
