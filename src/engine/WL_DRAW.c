/* SPDX-License-Identifier: GPL-2.0-only */
/* WL_DR_A.ASM quadrant initialization and vertcheck/horizcheck translated
   directly to bounded C; WL_DRAW.C wall/door hits, texture flips and height. */
#include "WL_DEF.h"
#include "WL_DEMO.h"
#include "ID_CA.h"
#include "ID_PM.h"
#include "ID_VL.h"
#include <string.h>
unsigned wallheight[MAXVIEWWIDTH];
static bool drawing_paused;
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
        if ((tile&0xc0)==0xc0) {
            /* WL_DR_A.ASM vertpushwall/horizpushwall: project the original
               0..63 partial step and pass rays that leave this tile. */
            int64_t mid=vertical ? yi+WL_FloorShift((int64_t)ys*pwallpos,6) :
                xi+WL_FloorShift((int64_t)xs*pwallpos,6);
            if (WL_FloorShift(mid,16)!=(vertical ? ty : tx)) pass=true;
            else {
                int32_t offset=(int32_t)(pwallpos<<10);
                hx=vertical ? xtile*65536+(dx<0 ? 65536-offset : offset) : (int32_t)mid;
                hy=vertical ? (int32_t)mid : ytile*65536+(dy<0 ? 65536-offset : offset);
                column=((uint32_t)mid>>10)&63;
                if (vertical ? dx<0 : dy>0) column=63-column;
                page=((tile&63)-1)*2+(vertical ? 1U : 0U);
                side=vertical ? 4 : 3;
            }
        } else if (tile&128) {
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
            /* Original wall tables leave index zero at page zero. A door
               beside floor marks that floor 0x40 (e.g. owned E2M3). */
            if (vertical) {
                hx=xtile*65536+(dx<0 ? 65536 : 0); hy=(int32_t)yi;
                column=((uint32_t)yi>>10)&63; if (dx<0) column=63-column;
                page=wall ? (wall-1)*2+1 : 0;
                if ((tile&64) && tx-dx>=0 && tx-dx<64 && (tilemap[tx-dx][ty]&128)) page=PMSpriteStart-8+3;
            } else {
                hx=(int32_t)xi; hy=ytile*65536+(dy<0 ? 65536 : 0);
                column=((uint32_t)xi>>10)&63; if (dy>0) column=63-column;
                page=wall ? (wall-1)*2 : 0;
                if ((tile&64) && ty-dy>=0 && ty-dy<64 && (tilemap[tx][ty-dy]&128)) page=PMSpriteStart-8+2;
            }
        }
        if (!pass) {
            if (page>=PMSpriteStart) return CA_Fail("Ray selected a non-wall texture");
            hit->x=hx;hit->y=hy;hit->page=page;hit->column=column;hit->height=CalcHeight(hx,hy);
            hit->side=side;hit->grid=side==2 ? (int)tile : vertical ? xtile : ytile;hit->tile=(int)tile;
            return true;
        }
        spotvis[tx][ty]=true;
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
/* WL_DRAW.C vgaCeiling: episode/map-specific original ceiling palette. */
static const w3r_byte vgaCeiling[60]={
    0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0xbf,
    0x4e,0x4e,0x4e,0x1d,0x8d,0x4e,0x1d,0x2d,0x1d,0x8d,
    0x1d,0x1d,0x1d,0x1d,0x1d,0x2d,0xdd,0x1d,0x1d,0x98,
    0x1d,0x9d,0x2d,0xdd,0xdd,0x9d,0x2d,0x4d,0x1d,0xdd,
    0x7d,0x1d,0x2d,0x2d,0xdd,0xd7,0x1d,0x1d,0x1d,0x2d,
    0x1d,0x1d,0x1d,0x1d,0xdd,0xdd,0x7d,0xdd,0xdd,0xdd,
};
bool ThreeDRefresh(void)
{
    memset(spotvis,0,sizeof(spotvis));
    /* DOS marks only cells crossed by rays, not the starting player cell. */
    for (unsigned y=0;y<VIEWHEIGHT;++y)
        for (unsigned x=0;x<VIEWWIDTH;++x)
            framebuffer[(VIEWY+y)*320+VIEWX+x]=(w3r_byte)(y<VIEWHEIGHT/2 ? vgaCeiling[gamestate.episode*10+gamestate.mapon] : 0x19);
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
        wallheight[x]=previous.height;
    }
    if (postwidth) ScalePost(postx,postwidth,previous.height,post);
    if (!DrawScaleds()) return false;
    static const unsigned weapons[4]={SPR_KNIFEREADY,SPR_PISTOLREADY,SPR_MACHINEGUNREADY,SPR_CHAINREADY};
    if (gamestate.died) return true;
    if (gamestate.victoryflag) {
        if (player.state==&s_deathcam && (player.temp1&32))
            return SimpleScaleShape(VIEWWIDTH/2,SPR_DEATHCAM,VIEWHEIGHT+1);
        return true;
    }
    return SimpleScaleShape(VIEWWIDTH/2,weapons[gamestate.weapon]+(unsigned)gamestate.weaponframe,VIEWHEIGHT+1) &&
        (!Demo_Active() || SimpleScaleShape(VIEWWIDTH/2,SPR_DEMO,VIEWHEIGHT+1));
}

/* WL_DRAW.C TransformActor/TransformTile: signed-magnitude rotation,
   original forward size bias, perspective divide and height numerator. */
static void Transform(int32_t x,int32_t y,int32_t bias,int *sx,int *height,int32_t *nx,int32_t *ny)
{
    SetView();
    int32_t gx=x-viewx,gy=y-viewy;
    *nx=FixedByFrac(gx,viewcos)-FixedByFrac(gy,viewsin)-bias;
    *ny=FixedByFrac(gy,viewcos)+FixedByFrac(gx,viewsin);
    if (*nx<MINDIST) { *height=0;return; }
    int scale=(VIEWWIDTH/2)*(FOCALLENGTH+MINDIST)/32768;
    *sx=VIEWWIDTH/2-1+(int)((int64_t)*ny*scale / *nx);
    *height=(65536*scale/64)/(*nx/256);
}
void TransformActor(objtype *ob)
{
    Transform(ob->x,ob->y,ACTORSIZE,&ob->viewx,&ob->viewheight,&ob->transx,&ob->transy);
}
bool TransformTile(int tx,int ty,int *x,int *height)
{
    int32_t nx,ny;
    Transform(tx*TILEGLOBAL+32768,ty*TILEGLOBAL+32768,0x2000,x,height,&nx,&ny);
    return *height && nx<TILEGLOBAL && ny>-TILEGLOBAL/2 && ny<TILEGLOBAL/2;
}
int CalcRotate(const objtype *ob)
{
    static const int dirangle[9]={0,45,90,135,180,225,270,315,360};
    int viewangle=player.angle+(VIEWWIDTH/2-1-ob->viewx)/8;
    int angle=(viewangle-180)-(ob->obclass==rocketobj ? ob->angle : dirangle[ob->dir])+ANGLES/16;
    angle%=ANGLES;if (angle<0) angle+=ANGLES;
    return ob->state->rotate==2 ? 4*(angle/(ANGLES/2)) : angle/(ANGLES/8);
}
bool DrawScaleds(void)
{
    /* Original MAXVISABLE limit and stable least-height selection. */
    struct { int x,height,shape; } vis[50];unsigned count=0;
    for (unsigned i=0;i<statcount;++i) {
        statobj_t *ob=&statobjlist[i];
        if (ob->shapenum<0 || !spotvis[ob->tilex][ob->tiley]) continue;
        int x=0,height=0;
        if (TransformTile(ob->tilex,ob->tiley,&x,&height) && ob->bonus && !gamestate.died && !drawing_paused) { GetBonus(ob);continue; }
        if (height && count<49) { vis[count].x=x;vis[count].height=height;vis[count++].shape=ob->shapenum; }
    }
    for (objtype *current=player.next;current;current=current->next) {
        objtype copy=*current,*ob=drawing_paused?&copy:current;
        if (!ob->state || !ob->state->shapenum) continue;
        bool visible=spotvis[ob->tilex][ob->tiley];
        for (int dx=-1;dx<=1;++dx) for (int dy=-1;dy<=1;++dy) {
            int x=ob->tilex+dx,y=ob->tiley+dy;
            if (x>=0 && y>=0 && x<64 && y<64 && !tilemap[x][y] && spotvis[x][y]) visible=true;
        }
        if (!visible) { ob->flags&=~FL_VISABLE;continue; }
        ob->active=true;TransformActor(ob);
        /* DOS retains the prior visible flag on the too-close branch. */
        if (!ob->viewheight) continue;
        ob->flags|=FL_VISABLE;
        if (count<49) {
            vis[count].x=ob->viewx;vis[count].height=ob->viewheight;
            vis[count++].shape=ob->state->shapenum+(ob->state->rotate ? CalcRotate(ob) : 0);
        }
    }
    for (unsigned i=0;i<count;++i) {
        unsigned farthest=0;
        for (unsigned j=1;j<count;++j) if (vis[j].height<vis[farthest].height) farthest=j;
        if (!ScaleShape(vis[farthest].x,(unsigned)vis[farthest].shape,(unsigned)vis[farthest].height)) return false;
        vis[farthest].height=32000;
    }
    return true;
}

bool ThreeDRefreshPaused(void)
{
    drawing_paused=true;bool ok=ThreeDRefresh();drawing_paused=false;return ok;
}
