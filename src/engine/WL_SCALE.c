/* SPDX-License-Identifier: GPL-2.0-only */
/* WL_SCALE.C BuildCompScale and WL_DRAW.C ScalePost without generated x86.
   Emit the original source-pixel runs, including scaler-directory quantization. */
#include "WL_DEF.h"
#include "ID_VL.h"
#include "ID_PM.h"
#include "ID_FILE.h"
void ScalePost(unsigned x,unsigned width,unsigned height,const w3r_byte *source)
{
    unsigned scale=height/8;
    unsigned maxscale=(unsigned)VIEWWIDTH*3/4-1;
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

/* Portable equivalent of BuildCompScale's source-pixel runs and ScaleLine's
   transparent posts. Palette index zero is opaque when the post says so. */
static bool DrawShape(int center,unsigned sprite,unsigned height,bool weapon)
{
    unsigned scale=weapon ? height/2 : height/8;
    if (!scale || scale>(unsigned)VIEWWIDTH*3/4-1) return true;
    if (scale>=VIEWHEIGHT/2) scale=VIEWHEIGHT/2+(scale-VIEWHEIGHT/2)/3*3;
    w3r_byte pixels[4096],opaque[4096];
    if (!PM_DecodeSprite(sprite,pixels,opaque)) return false;
    const w3r_byte *page=PM_GetPage(PMSpriteStart+sprite,NULL);
    int first=(int)CA_LE16(page),last=(int)CA_LE16(page+2);
    int size=(int)scale*2,top=(VIEWHEIGHT-size)/2;
    /* Original ScaleShape anchors both halves independently at xcenter.
       In particular, a shape wholly right of column 31 starts AT center;
       treating the decoded bitmap as a centered square shifts knife frames. */
    for (int half=0;half<2;++half) {
        int cursor=center;
        int start=half ? (first<31 ? 32 : first) : 31;
        int stop=half ? last : first;
        int step=half ? 1 : -1;
        for (int sx=start;half ? sx<=stop : sx>=stop;sx+=step) {
            int width=(sx+1)*size/64-sx*size/64;
            if (!half) cursor-=width;
            int x0=cursor,x1=cursor+width;
            if (half) cursor+=width;
            if (x0<0) x0=0;
            if (x1>VIEWWIDTH) x1=VIEWWIDTH;
            for (int sy=0;sy<64;++sy) {
                if (!opaque[sy*64+sx]) continue;
                int y0=top+sy*size/64,y1=top+(sy+1)*size/64;
                if (y0<0) y0=0;
                if (y1>VIEWHEIGHT) y1=VIEWHEIGHT;
                for (int x=x0;x<x1;++x) {
                    if (!weapon && wallheight[x]>=height) continue;
                    for (int y=y0;y<y1;++y) framebuffer[(VIEWY+y)*320+VIEWX+x]=pixels[sy*64+sx];
                }
            }
        }
    }
    return true;
}
bool ScaleShape(int x,unsigned sprite,unsigned height) { return DrawShape(x,sprite,height,false); }
bool SimpleScaleShape(int x,unsigned sprite,unsigned height) { return DrawShape(x,sprite,height,true); }
