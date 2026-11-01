/* SPDX-License-Identifier: GPL-2.0-only */
/* WL_DR_A.ASM quadrant initialization and vertcheck/horizcheck translated
   directly to bounded C; WL_DRAW.C wall/door hits, texture flips and height. */
#include "WL_DEF.h"
#include "ID_CA.h"
#include "ID_PM.h"
#include "ID_VL.h"
static int32_t viewx,viewy;
static uint32_t viewsin,viewcos;
static int32_t PartialStep(int32_t step,uint16_t partial)
{
    int64_t magnitude=step<0 ? -(int64_t)step : step;
    int32_t result=(int32_t)(magnitude*partial/65536);
    return step<0 ? -result : result;
}
static void SetView(void)
{
    viewsin=sintable[player.angle]; viewcos=sintable[player.angle+90];
    viewx=player.x-FixedByFrac(FOCALLENGTH,viewcos);
    viewy=player.y+FixedByFrac(FOCALLENGTH,viewsin);
}
static unsigned CalcHeight(int32_t x,int32_t y)
{
    int32_t nx=FixedByFrac(x-viewx,viewcos)-FixedByFrac(y-viewy,viewsin);
    if (nx<MINDIST) nx=MINDIST;
    int32_t scale=(VIEWWIDTH/2)*(FOCALLENGTH+MINDIST)/(65536/2);
    return (unsigned)((65536*scale/64)/(nx/256));
}
bool AsmRefresh(int angle,rayhit *hit)
{
    if (!hit || player.angle<0 || player.angle>=360) return CA_Fail("Invalid ray request");
    SetView(); angle%=3600; if (angle<0) angle+=3600;
    int dx,dy; int32_t xs,ys; uint16_t xp,yp;
    uint16_t xdown=(uint16_t)((uint32_t)viewx&65535),ydown=(uint16_t)((uint32_t)viewy&65535);
    uint16_t xup=(uint16_t)(65536U-xdown),yup=(uint16_t)(65536U-ydown);
    if (angle<900) { dx=1;dy=-1;xs=finetangent[899-angle];ys=-finetangent[angle];xp=xup;yp=ydown; }
    else if (angle<1800) { dx=-1;dy=-1;xs=-finetangent[angle-900];ys=-finetangent[1799-angle];xp=xdown;yp=ydown; }
    else if (angle<2700) { dx=-1;dy=1;xs=-finetangent[2699-angle];ys=finetangent[angle-1800];xp=xdown;yp=yup; }
    else { dx=1;dy=1;xs=finetangent[angle-2700];ys=finetangent[3599-angle];xp=xup;yp=yup; }
    int xtile=WL_FloorShift(viewx,16)+dx,ytile=WL_FloorShift(viewy,16)+dy;
    int64_t yi=(int64_t)viewy+PartialStep(ys,xp),xi=(int64_t)viewx+PartialStep(xs,yp);
    bool vertical=!(dy<0 ? WL_FloorShift(yi,16)<=ytile : WL_FloorShift(yi,16)>=ytile);
    for (unsigned crossings=0;crossings<256;++crossings) {
        int tx=vertical ? xtile : WL_FloorShift(xi,16);
        int ty=vertical ? WL_FloorShift(yi,16) : ytile;
        if (tx<0 || ty<0 || tx>=64 || ty>=64) return CA_Fail("Ray escaped map perimeter");
        unsigned tile=tilemap[tx][ty]; bool pass=tile==0;
        int32_t hx=0,hy=0; unsigned page=0,column=0; int side=vertical ? 1 : 0;
        if (tile&128) {
            unsigned door=tile&127;
            if (door>=doornum) return CA_Fail("Invalid door index in ray");
            int64_t mid=vertical ? yi+WL_FloorShift(ys,1) : xi+WL_FloorShift(xs,1);
            int before=vertical ? ty : tx;
            if (WL_FloorShift(mid,16)!=before || (uint16_t)mid<doorobjlist[door].position) pass=true;
            else {
                /* Original leading-edge test and texture offset for sliding doors. */
                hx=vertical ? xtile*65536+32768 : (int32_t)mid;
                hy=vertical ? (int32_t)mid : ytile*65536+32768;
                column=((uint32_t)((uint16_t)mid-doorobjlist[door].position)>>10)&63;
                unsigned lock=doorobjlist[door].lock;
                page=PMSpriteStart-8+(lock==0 ? 0 : lock==5 ? 4 : 6)+(vertical ? 1U : 0U);
                side=2;
            }
        } else if (tile) {
            unsigned wall=tile&63;
            if (!wall) return CA_Fail("Invalid empty door jamb");
            if (vertical) {
                hx=xtile*65536+(dx<0 ? 65536 : 0); hy=(int32_t)yi;
                column=((uint32_t)yi>>10)&63; if (dx<0) column=63-column;
                page=(wall-1)*2+1;
                if ((tile&64) && tx-dx>=0 && tx-dx<64 && (tilemap[tx-dx][ty]&128)) page=PMSpriteStart-8+3;
            } else {
                hx=(int32_t)xi; hy=ytile*65536+(dy<0 ? 65536 : 0);
                column=((uint32_t)xi>>10)&63; if (dy>0) column=63-column;
                page=(wall-1)*2;
                if ((tile&64) && ty-dy>=0 && ty-dy<64 && (tilemap[tx][ty-dy]&128)) page=PMSpriteStart-8+2;
            }
        }
        if (!pass) {
            if (page>=PMSpriteStart) return CA_Fail("Ray selected a non-wall texture");
            hit->x=hx;hit->y=hy;hit->page=page;hit->column=column;hit->height=CalcHeight(hx,hy);
            hit->side=side;hit->grid=side==2 ? (int)tile : vertical ? xtile : ytile;hit->tile=(int)tile;
            return true;
        }
        if (vertical) {
            xtile+=dx; yi+=ys;
            vertical=!(dy<0 ? WL_FloorShift(yi,16)<=ytile : WL_FloorShift(yi,16)>=ytile);
        } else {
            ytile+=dy; xi+=xs;
            vertical=dx<0 ? WL_FloorShift(xi,16)<=xtile : WL_FloorShift(xi,16)>=xtile;
        }
    }
    return CA_Fail("Ray traversal exceeded map bounds");
}
bool ThreeDRefresh(void)
{
    for (unsigned y=0;y<VIEWHEIGHT;++y)
        for (unsigned x=0;x<VIEWWIDTH;++x)
            framebuffer[(VIEWY+y)*320+VIEWX+x]=(w3r_byte)(y<VIEWHEIGHT/2 ? 0x1d : 0x19);
    rayhit previous={0}; unsigned postx=0,postwidth=0;
    const w3r_byte *post=NULL;
    for (unsigned x=0;x<VIEWWIDTH;++x) {
        rayhit hit;
        if (!AsmRefresh(player.angle*10+pixelangle[x],&hit)) return false;
        bool same=x && hit.side==previous.side && hit.grid==previous.grid && hit.tile==previous.tile && hit.page==previous.page;
        if (same && hit.column==previous.column) {
            ++postwidth; /* Original wide-post optimization reuses its first height. */
        } else {
            if (postwidth) ScalePost(postx,postwidth,previous.height,post);
            size_t length;
            const w3r_byte *page=PM_GetPage(hit.page,&length);
            if (!page || length!=4096) return CA_Fail("Invalid wall texture length");
            post=page+hit.column*64;postx=x;postwidth=1;previous=hit;
        }
    }
    if (postwidth) ScalePost(postx,postwidth,previous.height,post);
    return true;
}
