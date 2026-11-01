/* SPDX-License-Identifier: GPL-2.0-only */
/* WOLFSRC/WL_ACT1.C OpenDoor/CloseDoor/OperateDoor/MoveDoors, one tic at a
   time. Actor occupancy, area connectivity and positional audio await actors.
   Key inventory is not ported, so the original locked-door rejection remains. */
#include "WL_DEF.h"
bool DoorSolid(unsigned door) { return door>=doornum || doorobjlist[door].action!=dr_open; }
void OpenDoor(unsigned door)
{
    if (door>=doornum) return;
    doorobj_t *d=&doorobjlist[door];
    if (d->action==dr_open) d->ticcount=0;
    else d->action=dr_opening;
}
static bool PlayerBlocks(const doorobj_t *d)
{
    if (player.tilex==d->x && player.tiley==d->y) return true;
    if (d->vertical && player.tiley==d->y)
        return (unsigned)((player.x+MINDIST)/65536)==d->x || (unsigned)((player.x-MINDIST)/65536)==d->x;
    if (!d->vertical && player.tilex==d->x)
        return (unsigned)((player.y+MINDIST)/65536)==d->y || (unsigned)((player.y-MINDIST)/65536)==d->y;
    return false;
}
void CloseDoor(unsigned door)
{
    if (door>=doornum) return;
    doorobj_t *d=&doorobjlist[door];
    /* Original actorat marker stays solid throughout opening/closing. */
    if (DoorSolid(door) || PlayerBlocks(d)) return;
    d->action=dr_closing;
}
void OperateDoor(unsigned door)
{
    if (door>=doornum) return;
    doorobj_t *d=&doorobjlist[door];
    if (d->lock>=1 && d->lock<=4) return;
    if (d->action==dr_closed || d->action==dr_closing) OpenDoor(door);
    else CloseDoor(door);
}
void MoveDoors(void)
{
    for (unsigned i=0;i<doornum;++i) {
        doorobj_t *d=&doorobjlist[i];
        if (d->action==dr_open) {
            if (d->ticcount<300) ++d->ticcount;
            if (d->ticcount>=300) CloseDoor(i);
        } else if (d->action==dr_opening) {
            unsigned position=(unsigned)d->position+1024;
            if (position>=65535) { position=65535;d->ticcount=0;d->action=dr_open; }
            d->position=(uint16_t)position;
        } else if (d->action==dr_closing) {
            if (player.tilex==d->x && player.tiley==d->y) { OpenDoor(i);continue; }
            if (d->position<=1024) { d->position=0;d->action=dr_closed; }
            else d->position=(uint16_t)(d->position-1024);
        }
    }
}
/* WL_AGENT.C Cmd_Use cardinal facing and adjacent-tile lookup. Only doors
   are enabled: no pretend pushwall, elevator, pickup or progression action. */
void Cmd_Use(void)
{
    int x=(int)player.tilex,y=(int)player.tiley;
    if (player.angle<45 || player.angle>315) ++x;
    else if (player.angle<135) --y;
    else if (player.angle<225) --x;
    else ++y;
    if (x<0 || y<0 || x>=64 || y>=64) return;
    unsigned tile=tilemap[x][y];
    if (tile&128) OperateDoor(tile&127);
}
