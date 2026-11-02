/* SPDX-License-Identifier: GPL-2.0-only */
#include "WL_RENDER_TARGET.h"
#include <string.h>
#include <stdint.h>
#include <stddef.h>
bool WL_TargetValid(const wl_render_target *t)
{
    return t && t->pixels && t->width && t->height &&
        t->width<=WL_TARGET_MAX_WIDTH && t->height<=WL_TARGET_MAX_HEIGHT &&
        t->stride>=t->width && t->stride<=WL_TARGET_MAX_WIDTH*2 &&
        (size_t)t->stride<=SIZE_MAX/t->height &&
        (size_t)t->stride*t->height<=t->capacity;
}
bool WL_TargetInit(wl_render_target *target,uint8_t *pixels,size_t capacity,
                   unsigned width,unsigned height,unsigned stride)
{
    wl_render_target candidate={pixels,width,height,stride,capacity};
    if(!target || !WL_TargetValid(&candidate)) return false;
    *target=candidate;return true;
}
bool WL_TargetClear(const wl_render_target *target,uint8_t color)
{
    if(!WL_TargetValid(target)) return false;
    for(unsigned y=0;y<target->height;++y)
        memset(target->pixels+(size_t)y*target->stride,color,target->width);
    return true;
}
bool WL_TargetPixel(const wl_render_target *target,unsigned x,unsigned y,uint8_t color)
{
    if(!WL_TargetValid(target) || x>=target->width || y>=target->height) return false;
    target->pixels[(size_t)y*target->stride+x]=color;return true;
}
