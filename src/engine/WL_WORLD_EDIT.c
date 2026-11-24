/* SPDX-License-Identifier: GPL-2.0-only */
#include "WL_WORLD_EDIT.h"
#include <stdlib.h>

bool WL_EditWorldValid(const wl_edit_world *world)
{
    if(!world || !world->tiles || !world->actors || !world->blocked_statics ||
        !world->pushable || !world->player || world->width<=2 || world->height<=2 ||
        world->width>64 || world->height>64 || !world->wall_limit || world->wall_limit>63 ||
        world->static_count>MAXSTATS || (world->static_count && !world->statics)) return false;
    if(world->player->tilex<0 || world->player->tiley<0 ||
        (unsigned)world->player->tilex>=world->width || (unsigned)world->player->tiley>=world->height ||
        world->player->x<0 || world->player->y<0 ||
        (uint64_t)world->player->x>=(uint64_t)world->width*TILEGLOBAL ||
        (uint64_t)world->player->y>=(uint64_t)world->height*TILEGLOBAL) return false;
    return WL_FloorShift(world->player->x,16)==world->player->tilex &&
        WL_FloorShift(world->player->y,16)==world->player->tiley;
}

static bool InBounds(const wl_edit_world *world,int x,int y)
{ return x>=0 && y>=0 && (unsigned)x<world->width && (unsigned)y<world->height; }

static bool PlayerOverlaps(const objtype *subject,int x,int y)
{
    int64_t left=(int64_t)subject->x-MINDIST,right=(int64_t)subject->x+MINDIST;
    int64_t top=(int64_t)subject->y-MINDIST,bottom=(int64_t)subject->y+MINDIST;
    int64_t tile_left=(int64_t)x*TILEGLOBAL,tile_top=(int64_t)y*TILEGLOBAL;
    return right>tile_left && left<tile_left+TILEGLOBAL &&
        bottom>tile_top && top<tile_top+TILEGLOBAL;
}

static bool StaticOccupies(const wl_edit_world *world,int x,int y)
{
    if(world->blocked_statics[x][y]) return true;
    for(unsigned i=0;i<world->static_count;++i)
        if(world->statics[i].shapenum>=0 && world->statics[i].tilex==x && world->statics[i].tiley==y) return true;
    return false;
}

static bool PassableForEnclosure(const wl_edit_world *world,int x,int y,int removed_x,int removed_y)
{
    if(x==removed_x && y==removed_y) return true;
    unsigned tile=world->tiles[x][y];
    return !tile || tile==64 || (tile&0xc0)==0x80;
}

static bool RemovalKeepsEnclosure(const wl_edit_world *world,int removed_x,int removed_y)
{
    bool seen[64][64]={{false}};unsigned queue[4096],head=0,tail=0;
    queue[tail++]=(unsigned)(removed_y*64+removed_x);seen[removed_x][removed_y]=true;
    static const int dx[]={1,0,-1,0},dy[]={0,1,0,-1};
    while(head<tail) {
        unsigned cell=queue[head++];int x=(int)(cell%64),y=(int)(cell/64);
        if(!x || !y || (unsigned)x==world->width-1 || (unsigned)y==world->height-1) return false;
        for(unsigned i=0;i<4;++i) {
            int nx=x+dx[i],ny=y+dy[i];
            if(nx<0 || ny<0 || (unsigned)nx>=world->width || (unsigned)ny>=world->height || seen[nx][ny]) continue;
            if(!PassableForEnclosure(world,nx,ny,removed_x,removed_y)) continue;
            seen[nx][ny]=true;queue[tail++]=(unsigned)(ny*64+nx);
        }
    }
    return true;
}

static bool PlacementTrapsPlayer(const wl_edit_world *world,int x,int y)
{
    int px=world->player->tilex,py=world->player->tiley;
    if(abs(x-px)+abs(y-py)!=1) return false;
    static const int dx[]={1,0,-1,0},dy[]={0,1,0,-1};
    for(unsigned i=0;i<4;++i) {
        int nx=px+dx[i],ny=py+dy[i];
        if(nx==x && ny==y) continue;
        if(nx>0 && ny>0 && (unsigned)nx<world->width-1 && (unsigned)ny<world->height-1 &&
           (!world->tiles[nx][ny] || world->tiles[nx][ny]==64) && !world->actors[nx][ny] && !world->blocked_statics[nx][ny]) return false;
    }
    return true;
}

wl_edit_result WL_EditCanSetWall(const wl_edit_world *world,int x,int y,unsigned wall_id)
{
    if(!WL_EditWorldValid(world)) return WL_EDIT_INVALID_STATE;
    if(!InBounds(world,x,y)) return WL_EDIT_OUT_OF_BOUNDS;
    if(!wall_id || wall_id>world->wall_limit || wall_id==21 || wall_id==22) return WL_EDIT_INVALID_WALL;
    if(!x || !y || (unsigned)x==world->width-1 || (unsigned)y==world->height-1) return WL_EDIT_PROTECTED;
    if(world->tiles[x][y]) return WL_EDIT_OCCUPIED;
    if(world->actors[x][y] || StaticOccupies(world,x,y) || PlayerOverlaps(world->player,x,y)) return WL_EDIT_OCCUPIED;
    if(PlacementTrapsPlayer(world,x,y)) return WL_EDIT_PROTECTED;
    return WL_EDIT_OK;
}

wl_edit_result WL_EditCanRemoveWall(const wl_edit_world *world,int x,int y)
{
    if(!WL_EditWorldValid(world)) return WL_EDIT_INVALID_STATE;
    if(!InBounds(world,x,y)) return WL_EDIT_OUT_OF_BOUNDS;
    if(!x || !y || (unsigned)x==world->width-1 || (unsigned)y==world->height-1) return WL_EDIT_PROTECTED;
    unsigned tile=world->tiles[x][y];
    if(!tile || tile>63) return WL_EDIT_NOT_WALL;
    if(tile==21 || tile==22) return WL_EDIT_PROTECTED;
    if(world->pushable[x][y] || (world->push_state && world->push_x==x && world->push_y==y)) return WL_EDIT_PROTECTED;
    if(!RemovalKeepsEnclosure(world,x,y)) return WL_EDIT_PROTECTED;
    return WL_EDIT_OK;
}

wl_edit_result WL_EditSetWall(wl_edit_world *world,int x,int y,unsigned wall_id)
{
    wl_edit_result result=WL_EditCanSetWall(world,x,y,wall_id);
    if(result==WL_EDIT_OK) world->tiles[x][y]=(w3r_byte)wall_id;
    return result;
}

wl_edit_result WL_EditRemoveWall(wl_edit_world *world,int x,int y)
{
    wl_edit_result result=WL_EditCanRemoveWall(world,x,y);
    if(result==WL_EDIT_OK) world->tiles[x][y]=0;
    return result;
}

bool WL_EditTileBlocked(const wl_edit_world *world,int x,int y)
{
    if(!WL_EditWorldValid(world) || !InBounds(world,x,y)) return true;
    unsigned tile=world->tiles[x][y];
    if(tile && tile!=64) return true;
    return world->actors[x][y] || world->blocked_statics[x][y];
}

const char *WL_EditResultName(wl_edit_result result)
{
    static const char *const names[]={"OK","OUT OF BOUNDS","INVALID WALL","OCCUPIED","PROTECTED","NOT A WALL","INVALID STATE"};
    return (unsigned)result<sizeof(names)/sizeof(names[0])?names[result]:"UNKNOWN";
}
