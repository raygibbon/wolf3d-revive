/* SPDX-License-Identifier: GPL-2.0-only */
/* Native indexed presentation using the same ray intersections as DOS. The
   authoritative DOS pass runs unchanged; this pass has no simulation writes. */
#include "WL_RENDER.h"
#include "WL_DEMO.h"
#include "ID_CA.h"
#include "ID_FILE.h"
#include "ID_PM.h"
#include "ID_VL.h"
#include <math.h>
#include <string.h>
#include <limits.h>
bool WL_RenderResolution(unsigned w,unsigned h)
{ return (w==640 && h==480) || (w==1280 && h==720) || (w==1920 && h==1080); }
void WL_RenderContent(unsigned width,unsigned height,unsigned *x,unsigned *w)
{
    uint64_t scaled=(uint64_t)height*4/3;
    unsigned content=scaled>width?width:(unsigned)scaled;
    if(x) *x=(width-content)/2;
    if(w) *w=content;
}
typedef struct {
    wl_render_target *target;
    int x,y,w,h;
    double focal;
    int32_t camera_x,camera_y;
    uint32_t sine,cosine;
    int32_t *depth;
    void (*service)(void);
} view;
static void Project(const view *v,int32_t x,int32_t y,int32_t bias,int32_t *nx,int32_t *ny)
{
    int32_t dx=x-v->camera_x,dy=y-v->camera_y;
    *nx=FixedByFrac(dx,v->cosine)-FixedByFrac(dy,v->sine)-bias;
    *ny=FixedByFrac(dy,v->cosine)+FixedByFrac(dx,v->sine);
}
/* Retain original two-half anchoring and transparency posts. Index zero can
   be opaque. Square DOS texels become 1:1.2 on the corrected 4:3 canvas. */
static bool Shape(const view *v,double center,unsigned sprite,double width,int32_t distance,bool overlay)
{
    if(width<=0 || width>1000000) return true;
    w3r_byte pixels[4096],opaque[4096];
    if(!PM_DecodeSprite(sprite,pixels,opaque)) return false;
    const w3r_byte *page=PM_GetPage(PMSpriteStart+sprite,NULL);
    int first=(int)CA_LE16(page),last=(int)CA_LE16(page+2);
    double height=width*1.2,top=((double)v->h-height)/2;
    for(int half=0;half<2;++half) {
        double cursor=center;
        int start=half?(first<32?32:first):31,stop=half?last:first;
        for(int sx=start;half?sx<=stop:sx>=stop;sx+=half?1:-1) {
            if(v->service && sx%4==0) v->service();
            double span=(sx+1)*width/64-sx*width/64;
            if(!half) cursor-=span;
            int x0=(int)floor(cursor),x1=(int)floor(cursor+span);
            if(half) cursor+=span;
            if(x0<0) x0=0;
            if(x1>v->w) x1=v->w;
            for(int sy=0;sy<64;++sy) {
                if(!opaque[sy*64+sx]) continue;
                int y0=(int)floor(top+sy*height/64),y1=(int)floor(top+(sy+1)*height/64);
                if(y0<0) y0=0;
                if(y1>v->h) y1=v->h;
                for(int x=x0;x<x1;++x) {
                    if(!overlay && v->depth[x]<=distance) continue;
                    for(int y=y0;y<y1;++y)
                        v->target->pixels[(size_t)(v->y+y)*v->target->stride+(unsigned)(v->x+x)]=pixels[sy*64+sx];
                }
            }
        }
    }
    return true;
}
typedef struct { int32_t depth;double center,width;unsigned shape; } render_sprite;
static bool Scene(view *v)
{
    for(int x=0;x<v->w;++x) {
        if(v->service && x%16==0) v->service();
        /* Same field of view as the DOS viewport, sampled at each new column.
           Keep the original 0.1-degree tangent table and intersection rules. */
        double offset=((double)v->w/2-0.5-x)/v->focal;
        int angle=player.angle*10+(int)(atan(offset)*1800/3.141592657);
        rayhit hit;if(!WL_RenderRay(angle,&hit)) return false;
        int32_t nx,ny;Project(v,hit.x,hit.y,0,&nx,&ny);(void)ny;
        if(nx<MINDIST) nx=MINDIST;
        v->depth[x]=nx;
        double height=65536*v->focal/(double)nx*1.2,top=((double)v->h-height)/2;
        int start=(int)ceil(top),end=(int)ceil(top+height);
        if(start<0) start=0;
        if(end>v->h) end=v->h;
        size_t bytes;const w3r_byte *page=PM_GetPage(hit.page,&bytes);
        if(!page || bytes!=4096) return CA_Fail("Invalid high-resolution wall page");
        for(int y=0;y<v->h;++y) {
            unsigned color=y<v->h/2?WL_CeilingColor():0x19;
            if(y>=start && y<end) {
                int row=(int)(((double)y-top)*64/height);
                if(row<0) row=0;
                if(row>63) row=63;
                color=page[hit.column*64+(unsigned)row];
            }
            v->target->pixels[(size_t)(v->y+y)*v->target->stride+(unsigned)(v->x+x)]=(w3r_byte)color;
        }
    }
    render_sprite sprites[MAXSTATS+MAXACTORS];
    unsigned count=0;
    for(unsigned i=0;i<statcount;++i) {
        const statobj_t *o=&statobjlist[i];if(o->shapenum<0) continue;
        int32_t nx,ny;Project(v,o->tilex*TILEGLOBAL+32768,o->tiley*TILEGLOBAL+32768,0x2000,&nx,&ny);
        if(nx<MINDIST) continue;
        sprites[count].depth=nx;sprites[count].center=(double)v->w/2+(double)ny*v->focal/nx;
        sprites[count].width=65536*v->focal/nx;sprites[count++].shape=(unsigned)o->shapenum;
    }
    for(const objtype *o=player.next;o;o=o->next) {
        if(!o->state || !o->state->shapenum) continue;
        int32_t nx,ny;Project(v,o->x,o->y,ACTORSIZE,&nx,&ny);
        if(nx<MINDIST) continue;
        /* Rotate a local copy using the original angle rule; no actor writes. */
        objtype copy=*o;TransformActor(&copy);
        int shape=o->state->shapenum;
        if(shape<0) shape=o->temp1;
        if(o->state->rotate) shape+=CalcRotate(&copy);
        if(count>=MAXSTATS+MAXACTORS) return CA_Fail("High-resolution sprite list overflow");
        sprites[count].depth=nx;sprites[count].center=(double)v->w/2+(double)ny*v->focal/nx;
        sprites[count].width=65536*v->focal/nx;sprites[count++].shape=(unsigned)shape;
    }
    /* Stable far-to-near order; transparent posts preserve nearer artwork. */
    for(unsigned i=1;i<count;++i) {
        render_sprite item=sprites[i];unsigned j=i;
        while(j && sprites[j-1].depth<item.depth) { sprites[j]=sprites[j-1];--j; }
        sprites[j]=item;
    }
    for(unsigned i=0;i<count;++i)
        if(!Shape(v,sprites[i].center,sprites[i].shape,sprites[i].width,sprites[i].depth,false)) return false;
    static const unsigned weapons[4]={SPR_KNIFEREADY,SPR_PISTOLREADY,SPR_MACHINEGUNREADY,SPR_CHAINREADY};
    if(!gamestate.victoryflag && !Shape(v,(double)v->w/2,weapons[gamestate.weapon]+(unsigned)gamestate.weaponframe,(double)v->h/1.2,0,true)) return false;
    return !Demo_Active() || Shape(v,(double)v->w/2,SPR_DEMO,(double)v->h/1.2,0,true);
}
bool WL_RenderFrame(wl_render_target *target,int32_t *depth,size_t columns,bool world,void (*service)(void))
{
    if(!WL_TargetValid(target) || !WL_RenderResolution(target->width,target->height) || !depth || columns<target->width)
        return CA_Fail("Invalid high-resolution target/depth capacity");
    unsigned left,width;WL_RenderContent(target->width,target->height,&left,&width);
    if(!WL_TargetClear(target,0)) return false;
    /* Original UI, border and HUD retain the original pixel aspect correction. */
    for(unsigned y=0;y<target->height;++y) {
        if(service && y%32==0) service();
        for(unsigned x=0;x<width;++x)
            target->pixels[(size_t)y*target->stride+left+x]=framebuffer[(size_t)(y*200/target->height)*320+x*320/width];
    }
    if(!world || !CA_Map() || gamestate.died || gamestate.victoryflag) return true;
    view v={0};v.target=target;v.depth=depth;v.service=service;
    v.x=(int)(left+(unsigned)VIEWX*width/320);v.y=(int)((unsigned)VIEWY*target->height/200);
    v.w=(int)(((unsigned)VIEWX+VIEWWIDTH)*width/320)-(v.x-(int)left);
    v.h=(int)(((unsigned)VIEWY+VIEWHEIGHT)*target->height/200)-v.y;
    v.focal=(double)v.w/2*(FOCALLENGTH+MINDIST)/32768;
    v.sine=sintable[player.angle];v.cosine=sintable[player.angle+90];
    v.camera_x=player.x-FixedByFrac(FOCALLENGTH,v.cosine);v.camera_y=player.y+FixedByFrac(FOCALLENGTH,v.sine);
    return Scene(&v);
}
