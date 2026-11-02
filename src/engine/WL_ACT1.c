/* SPDX-License-Identifier: GPL-2.0-only */
/* WOLFSRC/WL_ACT1.C OpenDoor/CloseDoor/OperateDoor/MoveDoors, one tic at a
   time. Original marked-grid/center-row checks protect closing doors; key
   inventory gates locked doors. ConnectAreas rebuilds connectivity in the one-tic game loop. */
#include "WL_DEF.h"
#include <string.h>
static void DoorSound(soundnames sound,const doorobj_t *d)
{
    int area=areaMap[d->x][d->y];
    if(area<0 || area>=37 || !areabyplayer[area]) return;
    objtype position={0};position.x=(int32_t)d->x*TILEGLOBAL+32768;position.y=(int32_t)d->y*TILEGLOBAL+32768;
    GameSound(sound,&position);
}
bool DoorSolid(unsigned door) { return door>=doornum || doorobjlist[door].action!=dr_open; }
void OpenDoor(unsigned door)
{
    if (door>=doornum) return;
    doorobj_t *d=&doorobjlist[door];
    if (d->action==dr_open) d->ticcount=0;
    else d->action=dr_opening;
}
/* WL_ACT1.C CloseDoor uses the marked actor grid, including corpses. A
   non-shootable actor can still obstruct a doorway in the DOS simulation. */
static bool DoorOccupied(const doorobj_t *d)
{
    return actorat[d->x][d->y]!=NULL ||
        (player.tilex==(int)d->x && player.tiley==(int)d->y);
}
static bool PlayerBlocks(const doorobj_t *d)
{
    int x=(int)d->x,y=(int)d->y;
    if (DoorOccupied(d)) return true;
    if (d->vertical) {
        if (player.tiley==y && ((player.x+MINDIST)/TILEGLOBAL==x || (player.x-MINDIST)/TILEGLOBAL==x)) return true;
        const objtype *a=x>0?actorat[x-1][y]:NULL,*b=x<63?actorat[x+1][y]:NULL;
        return (a && (a->x+MINDIST)/TILEGLOBAL==x) || (b && (b->x-MINDIST)/TILEGLOBAL==x);
    }
    if (player.tilex==x && ((player.y+MINDIST)/TILEGLOBAL==y || (player.y-MINDIST)/TILEGLOBAL==y)) return true;
    const objtype *a=y>0?actorat[x][y-1]:NULL,*b=y<63?actorat[x][y+1]:NULL;
    return (a && (a->y+MINDIST)/TILEGLOBAL==y) || (b && (b->y-MINDIST)/TILEGLOBAL==y);
}

void CloseDoor(unsigned door)
{
    if (door>=doornum) return;
    doorobj_t *d=&doorobjlist[door];
    /* Original actorat marker stays solid throughout opening/closing. */
    if (DoorSolid(door) || PlayerBlocks(d)) return;
    DoorSound(CLOSEDOORSND,d);d->action=dr_closing;
}
void OperateDoor(unsigned door)
{
    if (door>=doornum) return;
    doorobj_t *d=&doorobjlist[door];
    if (d->lock>=1 && d->lock<=4 && !(gamestate.keys & (1 << (d->lock-1)))) { GameSound(NOWAYSND,NULL);return; }
    if (d->action==dr_closed || d->action==dr_closing) OpenDoor(door);
    else CloseDoor(door);
}
void MoveDoors(void)
{
    for (unsigned i=0;i<doornum;++i) {
        doorobj_t *d=&doorobjlist[i];
        if (d->action==dr_open) {
            /* Original signed 16-bit door timer, including blocked-door wrap. */
            d->ticcount=(d->ticcount+(unsigned)SimulationTics())&65535U;
            if (d->ticcount>=300 && d->ticcount<32768) CloseDoor(i);
        } else if (d->action==dr_opening) {
            if(!d->position) DoorSound(OPENDOORSND,d);
            unsigned position=(unsigned)d->position+1024U*(unsigned)SimulationTics();
            if (position>=65535) { position=65535;d->ticcount=0;d->action=dr_open;
                /* DOS DoorOpening releases the destination reservation. */
                actorat[d->x][d->y]=NULL; }
            d->position=(uint16_t)position;
        } else if (d->action==dr_closing) {
            if (DoorOccupied(d)) { OpenDoor(i);continue; }
            if (d->position<=1024U*(unsigned)SimulationTics()) { d->position=0;d->action=dr_closed; }
            else d->position=(uint16_t)(d->position-1024U*(unsigned)SimulationTics());
        }
    }
}
/* WL_AGENT.C Cmd_Use cardinal facing, doors, pushwalls and elevators. */
void Cmd_Use(void)
{
    int x=(int)player.tilex,y=(int)player.tiley;
    if (player.angle<45 || player.angle>315) ++x;
    else if (player.angle<135) --y;
    else if (player.angle<225) --x;
    else ++y;
    if (x<0 || y<0 || x>=64 || y>=64) return;
    unsigned tile=tilemap[x][y];
    int dir=player.angle<45 || player.angle>315 ? east : player.angle<135 ? north : player.angle<225 ? west : south;
    if (pushable[x][y]) { PushWall(x,y,dir);return; }
    if (tile==21 && (player.angle<45 || player.angle>315 || (player.angle>=135 && player.angle<225))) {
        GameSound(LEVELDONESND,NULL);
        ++tilemap[x][y]; /* Original elevator switch texture. */
        playstate=CA_Map()->planes[0][player.tiley*64+player.tilex]==107 ? ex_secretlevel : ex_completed;
    } else if (tile&128) OperateDoor(tile&127);
    else GameSound(DONOTHINGSND,NULL);
}

/* WL_ACT1.C PushWall/MovePWalls. Runtime marker/area arrays replace writes
   into cached map planes, so the disk map remains an immutable restart source. */
unsigned pwallstate,pwallpos;
int pwallx,pwally,pwalldir;
bool pushable[64][64];
static const int pushdx[8]={1,0,0,0,-1,0,0,0};
static const int pushdy[8]={0,0,-1,0,0,0,1,0};
void ResetPushwalls(void)
{
    pwallstate=pwallpos=0;pwallx=pwally=pwalldir=0;
    memset(pushable,0,sizeof(pushable));
}
static bool PushBlocked(int x,int y)
{
    return x<0 || y<0 || x>=64 || y>=64 || (tilemap[x][y] && tilemap[x][y]!=64) || staticblock[x][y] || actorat[x][y];
}
void PushWall(int x,int y,int dir)
{
    if (pwallstate || x<=0 || y<=0 || x>=63 || y>=63 ||
        dir<0 || dir>=8 || (dir&1) || !pushable[x][y]) return;
    unsigned tile=tilemap[x][y];
    if (!tile || tile>=64) return;
    if(PushBlocked(x+pushdx[dir],y+pushdy[dir])) { GameSound(NOWAYSND,NULL);return; }
    GameSound(PUSHWALLSND,NULL);
    tilemap[x+pushdx[dir]][y+pushdy[dir]]=(w3r_byte)tile;
    ++gamestate.secretcount;pwallx=x;pwally=y;pwalldir=dir;
    pwallstate=1;pwallpos=0;tilemap[x][y]|=0xc0;pushable[x][y]=false;
}
void MovePWalls(void)
{
    if (!pwallstate) return;
    unsigned oldblock=pwallstate/128;
    pwallstate+=(unsigned)SimulationTics();
    if (pwallstate/128!=oldblock) {
        unsigned tile=tilemap[pwallx][pwally]&63;
        tilemap[pwallx][pwally]=0;
        areaMap[pwallx][pwally]=areaMap[player.tilex][player.tiley];
        /* Stop after two tile crossings. Original >256 relied on multi-tic
           steps overshooting 256; >= also handles exact one-tic boundaries. */
        if (pwallstate>=256) { pwallstate=0;return; }
        pwallx+=pushdx[pwalldir];pwally+=pushdy[pwalldir];
        int nx=pwallx+pushdx[pwalldir],ny=pwally+pushdy[pwalldir];
        if (PushBlocked(nx,ny)) { pwallstate=0;return; }
        tilemap[nx][ny]=(w3r_byte)tile;
        tilemap[pwallx][pwally]=(w3r_byte)(tile|0xc0);
    }
    pwallpos=(pwallstate/2)&63U;
}
