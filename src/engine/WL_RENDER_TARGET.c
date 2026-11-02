/* SPDX-License-Identifier: GPL-2.0-only */
#include "WL_RENDER_TARGET.h"
#include <string.h>
#include <stdint.h>
#include <stddef.h>
bool WL_TargetInit(wl_render_target *target,uint8_t *pixels,size_t capacity,
                   unsigned width,unsigned height,unsigned stride)
{
    if(!target || !pixels || !width || !height || stride<width ||
       (size_t)stride>SIZE_MAX/(size_t)height ||
       (size_t)stride*(size_t)height>capacity) return false;
    target->pixels=pixels;target->width=width;
    target->height=height;target->stride=stride;return true;
}
bool WL_TargetClear(const wl_render_target *target,uint8_t color)
{
    if(!target || !target->pixels || !target->width || !target->height ||
       target->stride<target->width) return false;
    for(unsigned y=0;y<target->height;++y)
        memset(target->pixels+(size_t)y*target->stride,color,target->width);
    return true;
}
bool WL_TargetPixel(const wl_render_target *target,unsigned x,unsigned y,uint8_t color)
{
    if(!target || !target->pixels || x>=target->width || y>=target->height) return false;
    target->pixels[(size_t)y*target->stride+x]=color;return true;
}
