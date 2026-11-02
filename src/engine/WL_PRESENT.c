/* SPDX-License-Identifier: GPL-2.0-only */
#include "WL_PRESENT.h"
#include "WL_DEMO.h"
#include "WL_SAVE.h"
#include "ID_CA.h"
#include <string.h>
#include <math.h>
#include <stdlib.h>
static wl_present_snapshot previous,current;
static wl_frame_clock clock_state;
static uint64_t identities[MAXACTORS],next_identity,last_tick;
static bool ready;
void WL_PresentReset(void) { ready=false;WL_FrameClockReset(&clock_state); }
void WL_PresentAllocated(unsigned slot)
{ if(slot<MAXACTORS) { ++next_identity;if(!next_identity) ++next_identity;identities[slot]=next_identity; } }
static int32_t Lerp(int32_t a,int32_t b,double alpha)
{ return (int32_t)((double)a+((double)b-a)*alpha); }
double WL_PresentAngle(double a,double b,double alpha)
{
    double delta=fmod(b-a+540,360)-180,value=a+delta*alpha;
    if(value<0) value+=360;
    if(value>=360) value-=360;
    return value;
}
void WL_PresentCapture(uint64_t tick)
{
    if(!GameRunning() || !CA_Map()) { WL_PresentReset();return; }
    previous=current;memset(&current,0,sizeof(current));
    current.camera=(wl_pose){player.x,player.y,player.angle};
    memcpy(current.tiles,tilemap,sizeof(tilemap));memcpy(current.doors,doorobjlist,sizeof(doorobjlist));current.door_count=doornum;
    memcpy(current.statics,statobjlist,sizeof(statobjlist));current.static_count=statcount;
    current.push_x=pwallx;current.push_y=pwally;current.push_dir=pwalldir;current.push_state=pwallstate;current.push_offset=(int32_t)(pwallpos*1024);
    current.level=(unsigned)(gamestate.episode*10+gamestate.mapon);current.view_size=ViewSize();current.simulation_clock=gamestate.TimeCount;
    current.weapon=(unsigned)gamestate.weapon;current.weapon_frame=(unsigned)gamestate.weaponframe;
    current.died=gamestate.died;current.victory=gamestate.victoryflag;current.demo=Demo_Active();
    for(const objtype *o=player.next;o;o=o->next) {
        unsigned slot=(unsigned)(o-objlist);if(slot>=MAXACTORS || !o->state) continue;
        wl_present_actor *a=&current.actors[slot];
        a->identity=identities[slot];a->x=o->x;a->y=o->y;a->dir=o->dir;a->angle=o->angle;a->kind=o->obclass;
        a->shape=o->state->shapenum<0?o->temp1:o->state->shapenum;a->rotate=o->state->rotate;
        current.order[current.count++]=slot;
    }
    bool snap=!ready || current.level!=previous.level || current.view_size!=previous.view_size ||
        tick<=last_tick || current.victory ||
        current.died!=previous.died || current.victory!=previous.victory ||
        llabs((long long)current.camera.x-previous.camera.x)>TILEGLOBAL*2 ||
        llabs((long long)current.camera.y-previous.camera.y)>TILEGLOBAL*2;
    if(snap) { previous=current;WL_FrameClockReset(&clock_state); }
    WL_FrameClockObserve(&clock_state,WL_TickNS(tick));last_tick=tick;ready=true;
}
const wl_present_snapshot *WL_PresentCurrent(void) { return ready?&current:NULL; }
bool WL_PresentSample(uint64_t now_ns,wl_present_snapshot *out)
{
    if(!ready || !out) return false;
    *out=current;double alpha=WL_FrameClockAlpha(&clock_state,now_ns);
    out->camera.x=Lerp(previous.camera.x,current.camera.x,alpha);out->camera.y=Lerp(previous.camera.y,current.camera.y,alpha);
    out->camera.angle=WL_PresentAngle(previous.camera.angle,current.camera.angle,alpha);
    for(unsigned i=0;i<MAXACTORS;++i) if(current.actors[i].identity && current.actors[i].identity==previous.actors[i].identity) {
        const wl_present_actor *a=&previous.actors[i],*b=&current.actors[i];
        if(llabs((long long)b->x-a->x)>TILEGLOBAL*2 || llabs((long long)b->y-a->y)>TILEGLOBAL*2) continue;
        out->actors[i].x=Lerp(a->x,b->x,alpha);out->actors[i].y=Lerp(a->y,b->y,alpha);
        out->actors[i].angle=(int)WL_PresentAngle(a->angle,b->angle,alpha);
    }
    for(unsigned i=0;i<current.door_count;++i)
        out->doors[i].position=(uint16_t)Lerp(previous.doors[i].position,current.doors[i].position,alpha);
    if(previous.push_state && previous.push_dir==current.push_dir) {
        bool crossing=previous.push_x!=current.push_x || previous.push_y!=current.push_y || !current.push_state;
        int32_t end=crossing?65536+(!current.push_state?0:current.push_offset):current.push_offset;
        int32_t offset=Lerp(previous.push_offset,end,alpha);
        if(crossing && offset<65536) { memcpy(out->tiles,previous.tiles,sizeof(out->tiles));out->push_x=previous.push_x;out->push_y=previous.push_y; }
        out->push_offset=offset>=65536?offset-65536:offset;
    }
    return true;
}
