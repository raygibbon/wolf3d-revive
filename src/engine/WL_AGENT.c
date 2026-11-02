/* SPDX-License-Identifier: GPL-2.0-only */
/* WL_AGENT.C TryMove, ClipMove, Thrust and ControlMovement.
   Same MINDIST square, axis-slide order and movement/angle constants. */
#include "WL_DEF.h"
#include "ID_CA.h"
#include "ID_SD.h"
static int anglefrac;
void ResetMovement(void) { anglefrac=0; }
bool TryMove(const objtype *ob)
{
    int xl=WL_FloorShift((int64_t)ob->x-MINDIST,16),xh=WL_FloorShift((int64_t)ob->x+MINDIST,16);
    int yl=WL_FloorShift((int64_t)ob->y-MINDIST,16),yh=WL_FloorShift((int64_t)ob->y+MINDIST,16);
    if (xl<0 || yl<0 || xh>=64 || yh>=64) return false;
    for (int y=yl;y<=yh;++y) for (int x=xl;x<=xh;++x) if (tilemap[x][y] && tilemap[x][y]!=64) {
        unsigned tile=tilemap[x][y];
        if (!(tile&128) || DoorSolid(tile&127)) return false;
    }
    for (int y=yl;y<=yh;++y) for (int x=xl;x<=xh;++x) if (staticblock[x][y]) return false;
    for (unsigned i=0;i<objcount;++i) {
        const objtype *other=&objlist[i];
        if (!(other->flags&FL_SHOOTABLE)) continue;
        int64_t dx=(int64_t)ob->x-other->x,dy=(int64_t)ob->y-other->y;
        if (dx>=-MINACTORDIST && dx<=MINACTORDIST && dy>=-MINACTORDIST && dy<=MINACTORDIST) return false;
    }
    return true;
}
void ClipMove(objtype *ob,int32_t dx,int32_t dy)
{
    int32_t x=ob->x,y=ob->y;
    /* Internal thrust deltas are bounded; wide checks also protect this API. */
    int64_t nx=(int64_t)x+dx,ny=(int64_t)y+dy;
    if (nx<0 || ny<0 || nx>=64*65536 || ny>=64*65536) return;
    ob->x=(int32_t)nx; ob->y=(int32_t)ny; if (TryMove(ob)) return;
    sd_status audio;SD_GetStatus(&audio);
    if(audio.sound<0) GameSound(HITWALLSND,NULL);
    ob->y=y; if (TryMove(ob)) return;
    ob->x=x; ob->y=(int32_t)ny; if (TryMove(ob)) return;
    ob->x=x; ob->y=y;
}
void Thrust(int angle,int32_t speed)
{
    thrustspeed+=speed;
    if (speed>=MINDIST*2) speed=MINDIST*2-1;
    int32_t dx=FixedByFrac(speed,sintable[angle+90]);
    int32_t dy=-FixedByFrac(speed,sintable[angle]);
    ClipMove(&player,dx,dy);
    player.tilex=player.x/65536; player.tiley=player.y/65536;
    if (CA_Map()->planes[1][player.tiley*64+player.tilex]==99) (void)VictoryTile();
}
void ControlMovement(int controlx,int controly,bool strafe)
{
    if (strafe) {
        if (controlx>0) Thrust((player.angle+270)%360,controlx*150);
        else if (controlx<0) Thrust((player.angle+90)%360,-controlx*150);
    } else {
        anglefrac+=controlx;
        int units=anglefrac/20; anglefrac-=units*20;
        player.angle=(player.angle-units+360)%360;
    }
    if (controly<0) Thrust(player.angle,-controly*150);
    else if (controly>0) Thrust((player.angle+180)%360,controly*100);
}

int Save_GetAngleFraction(void) { return anglefrac; }
void Save_SetAngleFraction(int fraction) { anglefrac=fraction; }
