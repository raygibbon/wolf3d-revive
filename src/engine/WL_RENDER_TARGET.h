/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef WL_RENDER_TARGET_H
#define WL_RENDER_TARGET_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
/* A renderer-owned indexed surface. Classic continues to use its original
   fixed 320x200 framebuffer. The high-resolution renderer may use this type
   without requiring game simulation or SDL to know the output dimensions. */
typedef struct {
    uint8_t *pixels;
    unsigned width,height,stride;
} wl_render_target;
/* Does not allocate or own pixels. Stride is in pixels, not bytes. */
bool WL_TargetInit(wl_render_target *target,uint8_t *pixels,size_t capacity,
                   unsigned width,unsigned height,unsigned stride);
bool WL_TargetClear(const wl_render_target *target,uint8_t color);
bool WL_TargetPixel(const wl_render_target *target,unsigned x,unsigned y,uint8_t color);
#endif
