/* SPDX-License-Identifier: GPL-2.0-only */
/* WL_SCALE.C BuildCompScale and WL_DRAW.C ScalePost without generated x86.
   Emit the original source-pixel runs, including scaler-directory quantization. */
#include "WL_DEF.h"
#include "ID_VL.h"
void ScalePost(unsigned x,unsigned width,unsigned height,const w3r_byte *source)
{
    unsigned scale=height/8;
    unsigned maxscale=VIEWWIDTH*3/4-1;
    if (scale>maxscale) scale=maxscale;
    if (!scale) scale=1;
    unsigned stepbytwo=VIEWHEIGHT/2;
    if (scale>=stepbytwo) scale=stepbytwo+(scale-stepbytwo)/3*3;
    int pixels=(int)scale*2,top=(VIEWHEIGHT-pixels)/2;
    int32_t step=pixels*65536/64,fix=0;
    for (unsigned row=0;row<64;++row) {
        int start=fix/65536+top; fix+=step; int end=fix/65536+top;
        if (start<0) start=0;
        if (end>VIEWHEIGHT) end=VIEWHEIGHT;
        for (int y=start;y<end;++y) for (unsigned col=x;col<x+width && col<VIEWWIDTH;++col)
            framebuffer[(VIEWY+(unsigned)y)*320+VIEWX+col]=source[row];
    }
}
