/* SPDX-License-Identifier: GPL-2.0-only */
#include "WL_ASPECT.h"
#include <math.h>
#include <stdint.h>
#include <limits.h>
bool WL_AspectFit(unsigned cw,unsigned ch,unsigned aw,unsigned ah,wl_rectangle *o)
{
    if(!o || !cw || !ch || !aw || !ah) return false;
    uint64_t w,h;
    if((uint64_t)cw*ah <= (uint64_t)ch*aw) {
        w=cw;h=(uint64_t)cw*ah/aw;
    } else {
        h=ch;w=(uint64_t)ch*aw/ah;
    }
    if(!w || !h || w>UINT_MAX || h>UINT_MAX) return false;
    o->width=(unsigned)w;o->height=(unsigned)h;
    o->x=(cw-o->width)/2;o->y=(ch-o->height)/2;return true;
}
bool WL_HorizontalFov(double reference,unsigned aw,unsigned ah,double *out)
{
    if(!out || !aw || !ah || !(reference>0.0 && reference<179.0)) return false;
    const double pi=3.14159265358979323846;
    const double radians=reference*pi/180.0;
    const double ratio=((double)aw/(double)ah)/(4.0/3.0);
    double fov=2.0*atan(tan(radians/2.0)*ratio)*180.0/pi;
    if(!(fov>0.0 && fov<180.0)) return false;
    *out=fov;return true;
}

bool WL_AutoRenderSize(unsigned display_w,unsigned display_h,unsigned *width,unsigned *height)
{
    if(!width || !height || display_w<320 || display_h<200) return false;
    unsigned cap_w=display_w<1920U?display_w:1920U;
    unsigned cap_h=display_h<1080U?display_h:1080U;
    wl_rectangle fit;
    if(!WL_AspectFit(cap_w,cap_h,display_w,display_h,&fit) ||
       fit.width<320U || fit.height<200U) return false;
    *width=fit.width;*height=fit.height;
    return true;
}
