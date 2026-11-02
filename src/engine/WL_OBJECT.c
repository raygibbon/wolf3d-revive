/* SPDX-License-Identifier: GPL-2.0-only */
/* WL_GAME.C ScanInfoPlane; WL_ACT1.C SpawnStatic/PlaceItemType;
   WL_STATE.C SpawnNewObj/TryWalk, using bounded arrays instead of DOS tags.
   Normal enemies use the original difficulty bands and hit-point tables;
   Hans, Gretel and ghosts retain their specialised original spawn rules. */
#include "WL_DEF.h"
#include "ID_SD.h"
#include "ID_CA.h"
#include "ID_US_1.h"
#include <stdlib.h>
#include <string.h>
objtype objlist[MAXACTORS],*actorat[64][64];
statobj_t statobjlist[MAXSTATS];
unsigned objcount,statcount;
static bool allocated[MAXACTORS],allocationfailed;
bool staticblock[64][64],spotvis[64][64],areabyplayer[37],madenoise;
int areaMap[64][64],thrustspeed;
gametype gamestate={.health=100,.ammo=8,.lives=3,.nextextra=40000,.weapon=wp_pistol,.bestweapon=wp_pistol,.chosenweapon=wp_pistol,.difficulty=2};
extern statetype s_grdstand,s_grdpath1,s_grddie4,s_ofcstand,s_ofcpath1,s_ssstand,s_sspath1,s_mutstand,s_mutpath1,s_dogpath1;
extern statetype s_bossstand,s_gretelstand,s_blinkychase1,s_clydechase1,s_pinkychase1,s_inkychase1;
static void (*soundhook)(soundnames,const objtype *);
void SetGameSoundHook(void (*hook)(soundnames,const objtype *)) { soundhook=hook; }
static int32_t soundx,soundy;
static bool soundpositioned;
/* WL_GAME.C SetSoundLoc. Original tables and camera focal offset. */
static const unsigned char lefttable[15][30] = {
{ 8, 8, 8, 8, 8, 8, 8, 8, 5, 3, 1, 0, 0, 0, 0, 0, 6, 7, 7, 7, 7, 7, 7, 8, 8, 8, 8, 8, 8, 8},
{ 8, 8, 8, 8, 8, 8, 8, 8, 6, 4, 2, 0, 0, 0, 0, 0, 4, 6, 7, 7, 7, 7, 7, 8, 8, 8, 8, 8, 8, 8},
{ 8, 8, 8, 8, 8, 8, 8, 8, 6, 4, 2, 1, 0, 0, 0, 1, 4, 6, 6, 7, 7, 7, 7, 8, 8, 8, 8, 8, 8, 8},
{ 8, 8, 8, 8, 8, 8, 8, 8, 7, 5, 3, 2, 1, 0, 1, 2, 4, 5, 6, 7, 7, 7, 7, 8, 8, 8, 8, 8, 8, 8},
{ 8, 8, 8, 8, 8, 8, 8, 8, 8, 6, 5, 3, 3, 2, 2, 3, 4, 5, 6, 7, 7, 7, 8, 8, 8, 8, 8, 8, 8, 8},
{ 8, 8, 8, 8, 8, 8, 8, 8, 8, 7, 6, 5, 4, 4, 4, 4, 5, 6, 6, 7, 7, 7, 8, 8, 8, 8, 8, 8, 8, 8},
{ 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 7, 6, 6, 5, 5, 5, 6, 6, 7, 7, 7, 8, 8, 8, 8, 8, 8, 8, 8, 8},
{ 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 7, 7, 6, 6, 7, 7, 7, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8},
{ 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8},
{ 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8},
{ 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8},
{ 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8},
{ 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8},
{ 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8},
{ 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8}
};
static const unsigned char righttable[15][30] = {
{ 8, 8, 8, 8, 8, 8, 8, 7, 7, 7, 7, 7, 7, 6, 0, 0, 0, 0, 0, 1, 3, 5, 8, 8, 8, 8, 8, 8, 8, 8},
{ 8, 8, 8, 8, 8, 8, 8, 7, 7, 7, 7, 7, 6, 4, 0, 0, 0, 0, 0, 2, 4, 6, 8, 8, 8, 8, 8, 8, 8, 8},
{ 8, 8, 8, 8, 8, 8, 8, 7, 7, 7, 7, 6, 6, 4, 1, 0, 0, 0, 1, 2, 4, 6, 8, 8, 8, 8, 8, 8, 8, 8},
{ 8, 8, 8, 8, 8, 8, 8, 7, 7, 7, 7, 6, 5, 4, 2, 1, 0, 1, 2, 3, 5, 7, 8, 8, 8, 8, 8, 8, 8, 8},
{ 8, 8, 8, 8, 8, 8, 8, 8, 7, 7, 7, 6, 5, 4, 3, 2, 2, 3, 3, 5, 6, 8, 8, 8, 8, 8, 8, 8, 8, 8},
{ 8, 8, 8, 8, 8, 8, 8, 8, 7, 7, 7, 6, 6, 5, 4, 4, 4, 4, 5, 6, 7, 8, 8, 8, 8, 8, 8, 8, 8, 8},
{ 8, 8, 8, 8, 8, 8, 8, 8, 8, 7, 7, 7, 6, 6, 5, 5, 5, 6, 6, 7, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8},
{ 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 7, 7, 7, 6, 6, 7, 7, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8},
{ 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8},
{ 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8},
{ 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8},
{ 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8},
{ 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8},
{ 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8},
{ 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8}
};
void GameSoundPosition(int32_t gx,int32_t gy,unsigned *left,unsigned *right)
{
    if(player.angle<0 || player.angle>=360) { *left=*right=0;return; }
    uint32_t vs=sintable[player.angle],vc=sintable[player.angle+90];
    int32_t vx=player.x-FixedByFrac(FOCALLENGTH,vc),vy=player.y+FixedByFrac(FOCALLENGTH,vs);
    gx-=vx;gy-=vy;
    int x=WL_FloorShift((int64_t)FixedByFrac(gx,vc)-FixedByFrac(gy,vs),16);
    int y=WL_FloorShift((int64_t)FixedByFrac(gy,vc)+FixedByFrac(gx,vs),16);
    if(x<0) x=-x;
    if(x>=15) x=14;
    if(y>=15) y=14;
    if(y< -15) y=-15;
    *left=lefttable[x][y+15];*right=righttable[x][y+15];
}
void UpdateSoundLoc(void)
{
    if(soundpositioned) { unsigned left,right;GameSoundPosition(soundx,soundy,&left,&right);SD_SetPosition(left,right); }
}
void GameSound(soundnames sound,const objtype *actor)
{
    unsigned left=0,right=0;
    if(actor) GameSoundPosition(actor->x,actor->y,&left,&right);
    if(SD_PlaySound(sound,left,right) && SD_IsDigitizedSound(sound)) { soundpositioned=actor!=NULL;
        if(actor) { soundx=actor->x;soundy=actor->y; } }
    if (soundhook) soundhook(sound,actor);
}
void InitObjects(void)
{
    objcount=statcount=0;player.next=NULL;player.state=NULL;madenoise=false;
    memset(allocated,0,sizeof(allocated));allocationfailed=false;
    memset(objlist,0,sizeof(objlist));memset(actorat,0,sizeof(actorat));
    memset(staticblock,0,sizeof(staticblock));memset(spotvis,0,sizeof(spotvis));
    memset(areabyplayer,0,sizeof(areabyplayer));
}
/* WL_PLAY.C GetNewActor/RemoveObj, with an explicit bounded failure path.
   objcount is the occupied-array high-water mark, not the linked-list count. */
bool ActorAllocationFailed(void) { return allocationfailed; }
objtype *GetNewActor(void)
{
    unsigned slot=0;while (slot<MAXACTORS && allocated[slot]) ++slot;
    if (slot==MAXACTORS) { allocationfailed=true;(void)CA_Fail("Actor pool exhausted");return NULL; }
    objtype *ob=&objlist[slot];allocated[slot]=true;
    memset(ob,0,sizeof(*ob));
    if (slot>=objcount) objcount=slot+1;
    objtype *tail=&player;while (tail->next) tail=tail->next;
    tail->next=ob;return ob;
}
void RemoveActor(objtype *ob)
{
    if (!ob || ob==&player) return;
    objtype *prev=&player;while (prev->next && prev->next!=ob) prev=prev->next;
    if (prev->next!=ob) return;
    prev->next=ob->next;
    for (int x=0;x<64;++x) for (int y=0;y<64;++y)
        if (actorat[x][y]==ob) actorat[x][y]=NULL;
    allocated[ob-objlist]=false;ob->state=NULL;ob->flags=0;
}
objtype *SpawnActor(int x,int y,statetype *state)
{
    if (x<1 || y<1 || x>62 || y>62) {
        allocationfailed=true;(void)CA_Fail("Invalid actor position");return NULL;
    }
    objtype *ob=GetNewActor();if (!ob) return NULL;
    ob->state=state;ob->ticcount=state->tictime ? US_RndT()%state->tictime : 0;
    ob->x=x*TILEGLOBAL+32768;ob->y=y*TILEGLOBAL+32768;
    ob->tilex=x;ob->tiley=y;ob->areanumber=areaMap[x][y];ob->dir=nodir;
    actorat[x][y]=ob;return ob;
}
void NewState(objtype *ob,statetype *state) { ob->state=state;ob->ticcount=state->tictime; }
static const int dirx[9]={1,1,0,-1,-1,-1,0,1,0};
static const int diry[9]={0,-1,-1,-1,0,1,1,1,0};
static bool CheckTile(int x,int y,bool diagonal,int *door)
{
    if (x<=0 || y<=0 || x>=63 || y>=63 || staticblock[x][y]) return false;
    if (actorat[x][y] && (actorat[x][y]->flags&FL_SHOOTABLE)) return false;
    unsigned tile=tilemap[x][y];
    if (!tile || tile==64) return true;
    if (!(tile&128) || (tile&64)) return false;
    if (!DoorSolid(tile&63)) return true;
    if (diagonal) return false;
    *door=(int)(tile&63);return true;
}
bool TryWalk(objtype *ob)
{
    if (ob->dir<0 || ob->dir>=nodir) return false;
    int dx=dirx[ob->dir],dy=diry[ob->dir],x=ob->tilex+dx,y=ob->tiley+dy,door=-1;
    bool diag=dx && dy;
    if (!CheckTile(x,y,diag || ob->obclass==dogobj || ob->obclass==fakeobj,&door)) return false;
    if (diag && (!CheckTile(ob->tilex+dx,ob->tiley,true,&door) ||
                 !CheckTile(ob->tilex,ob->tiley+dy,true,&door))) return false;
    ob->tilex=x;ob->tiley=y;
    if (door>=0) { OpenDoor((unsigned)door);ob->distance=-door-1;return true; }
    ob->areanumber=areaMap[x][y];ob->distance=TILEGLOBAL;return true;
}
/* Original area graph semantics: a door connects its two areas as soon as
   its first opening tic advances, and disconnects only after it closes. Rebuilding
   the small graph avoids stale counters during level restarts. */
void ConnectAreas(void)
{
    memset(areabyplayer,0,sizeof(areabyplayer));
    int area=areaMap[player.tilex][player.tiley];
    areabyplayer[area]=true;
    for (unsigned pass=0;pass<37;++pass) {
        bool changed=false;
        for (unsigned i=0;i<doornum;++i) {
            const doorobj_t *d=&doorobjlist[i];
            if (d->action==dr_closed || (d->action==dr_opening && !d->position)) continue;
            int a=areaMap[d->x-(d->vertical?1U:0U)][d->y-(d->vertical?0U:1U)];
            int b=areaMap[d->x+(d->vertical?1U:0U)][d->y+(d->vertical?0U:1U)];
            if (areabyplayer[a]!=areabyplayer[b]) { areabyplayer[a]=areabyplayer[b]=true;changed=true; }
        }
        if (!changed) break;
    }
}

static const struct { int picnum;stat_t type; } statinfo[] = {
{SPR_STAT_0,dressing},
{SPR_STAT_1,block},
{SPR_STAT_2,block},
{SPR_STAT_3,block},
{SPR_STAT_4,dressing},
{SPR_STAT_5,block},
{SPR_STAT_6,bo_alpo},
{SPR_STAT_7,block},
{SPR_STAT_8,block},
{SPR_STAT_9,dressing},
{SPR_STAT_10,block},
{SPR_STAT_11,block},
{SPR_STAT_12,block},
{SPR_STAT_13,block},
{SPR_STAT_14,dressing},
{SPR_STAT_15,dressing},
{SPR_STAT_16,block},
{SPR_STAT_17,block},
{SPR_STAT_18,block},
{SPR_STAT_19,dressing},
{SPR_STAT_20,bo_key1},
{SPR_STAT_21,bo_key2},
{SPR_STAT_22,block},
{SPR_STAT_23,dressing},
{SPR_STAT_24,bo_food},
{SPR_STAT_25,bo_firstaid},
{SPR_STAT_26,bo_clip},
{SPR_STAT_27,bo_machinegun},
{SPR_STAT_28,bo_chaingun},
{SPR_STAT_29,bo_cross},
{SPR_STAT_30,bo_chalice},
{SPR_STAT_31,bo_bible},
{SPR_STAT_32,bo_crown},
{SPR_STAT_33,bo_fullheal},
{SPR_STAT_34,bo_gibs},
{SPR_STAT_35,block},
{SPR_STAT_36,block},
{SPR_STAT_37,block},
{SPR_STAT_38,bo_gibs},
{SPR_STAT_39,block},
{SPR_STAT_40,block},
{SPR_STAT_41,dressing},
{SPR_STAT_42,dressing},
{SPR_STAT_43,dressing},
{SPR_STAT_44,dressing},
{SPR_STAT_45,block},
{SPR_STAT_46,block},
{SPR_STAT_47,dressing},
{SPR_STAT_26,bo_clip2},
{-1,dressing}
};

static bool SpawnStatic(int x,int y,unsigned type)
{
    if (type>=48 || statcount>=MAXSTATS) return CA_Fail("Too many/invalid static objects");
    if (statinfo[type].type==block && actorat[x][y] && (actorat[x][y]->flags&FL_SHOOTABLE))
        return CA_Fail("Blocking object on actor destination");
    statobj_t *ob=&statobjlist[statcount++];
    ob->tilex=x;ob->tiley=y;ob->shapenum=statinfo[type].picnum;ob->itemnumber=statinfo[type].type;
    ob->bonus=ob->itemnumber>=bo_gibs;
    if (ob->itemnumber==block) staticblock[x][y]=true;
    if ((ob->itemnumber>=bo_cross && ob->itemnumber<=bo_crown) || ob->itemnumber==bo_fullheal) ++gamestate.treasuretotal;
    return true;
}
void PlaceItemType(stat_t item,int x,int y)
{
    if (x<=0 || y<=0 || x>=63 || y>=63) return;
    unsigned type=0;
    while (statinfo[type].picnum!=-1 && statinfo[type].type!=item) ++type;
    if (statinfo[type].picnum==-1) return;
    unsigned slot=0;
    while (slot<statcount && statobjlist[slot].shapenum!=-1) ++slot;
    if (slot>=MAXSTATS) return;
    if (slot==statcount) ++statcount;
    statobjlist[slot]=(statobj_t){x,y,statinfo[type].picnum,item,true};
}
static bool SpawnNewObj(int x,int y,statetype *state,int kind,int direction,bool patrol,bool dead)
{
    if (objcount>=MAXACTORS || x<=0 || y<=0 || x>=63 || y>=63 || (tilemap[x][y] && tilemap[x][y]!=64)) return CA_Fail("Invalid/too many actors");
    objtype *ob=SpawnActor(x,y,state);if (!ob) return false;
    ob->dir=direction*2;ob->areanumber=areaMap[x][y];ob->obclass=dead ? inertobj : kind;
    if (!dead) {
        ob->flags=FL_SHOOTABLE;
        if (!patrol && CA_Map()->planes[0][y*64+x]==106) ob->flags|=FL_AMBUSH;
        ob->speed=kind==dogobj ? SPDDOG : SPDPATROL;
        ob->hitpoints=kind==guardobj ? 25 : kind==officerobj ? 50 : kind==ssobj ? 100 : kind==dogobj ? 1 : gamestate.difficulty==0 ? 45 : gamestate.difficulty==3 ? 65 : 55;
        ++gamestate.killtotal;
    }
    if (patrol) {
        /* SpawnPatrol reserves the initial forward tile unconditionally,
           even a closed door (owned E4M6 on hard). Keep that original move. */
        actorat[x][y]=NULL; /* Original SpawnPatrol releases its birth tile. */
        ob->active=true;ob->distance=TILEGLOBAL;
        ob->tilex+=dirx[ob->dir];ob->tiley+=diry[ob->dir];
        if (ob->tilex<=0 || ob->tiley<=0 || ob->tilex>=63 || ob->tiley>=63)
            return CA_Fail("Invalid actor patrol destination");
    }
    /* Original scan order can overwrite a reserved destination (E5M4).
       Both actors remain in the object list; the last mark owns actorat. */
    actorat[ob->tilex][ob->tiley]=ob;return true;
}
/* WL_ACT2.C original boss spawning and SpawnGhosts. Ghosts intentionally
   have no FL_SHOOTABLE or health assignment, despite the unused HP table. */
static bool SpawnSpecial(int x,int y,unsigned tile)
{
    static const int health[4]={850,950,1050,1200};
    static statetype *const ghosts[]={&s_blinkychase1,&s_clydechase1,&s_pinkychase1,&s_inkychase1};
    bool ghost=tile>=224;
    statetype *state=NULL;int kind=ghostobj,dir=east,hp=0;
    if (ghost) state=ghosts[tile-224];
    else {
        hp=health[gamestate.difficulty];
        switch (tile) {
        case 214:state=&s_bossstand;kind=bossobj;dir=south;break;
        case 197:state=&s_gretelstand;kind=gretelobj;dir=north;break;
        case 196: {
            static const int schabbhp[]={850,950,1550,2400};
            state=&s_schabbstand;kind=schabbobj;dir=south;hp=schabbhp[gamestate.difficulty];break;
        }
        case 215:state=&s_giftstand;kind=giftobj;dir=north;break;
        case 179:state=&s_fatstand;kind=fatobj;dir=south;break;
        case 160:state=&s_fakestand;kind=fakeobj;dir=north;hp=200+100*gamestate.difficulty;break;
        case 178: {
            static const int mecha[]={800,950,1050,1200};
            state=&s_mechastand;kind=mechahitlerobj;dir=south;hp=mecha[gamestate.difficulty];break;
        }
        }
    }
    if (!state || !SpawnNewObj(x,y,state,kind,0,false,false)) return false;
    objtype *ob=&objlist[objcount-1];
    ob->flags=FL_AMBUSH|(ghost ? 0U : FL_SHOOTABLE);ob->dir=dir;
    ob->speed=ghost ? SPDDOG : SPDPATROL;ob->hitpoints=hp;
    /* Original logical DigiMode selects timing, never mute or device status. */
    s_schabbdie2.tictime=s_giftdie2.tictime=s_fatdie2.tictime=s_hitlerdie2.tictime=SD_DigitizedMode()?140:5;
    return true;
}
bool ScanInfoPlane(void)
{
    const maptype *map=CA_Map();
    for (int y=0;y<64;++y) for (int x=0;x<64;++x) {
        unsigned raw=map->planes[0][y*64+x];
        if (raw==106) {
            /* Original SpawnStand neighbour priority: E, N, S, then W. */
            const int ox[4]={1,0,0,-1},oy[4]={0,-1,1,0};raw=107;
            for (unsigned i=0;i<4;++i) {
                int nx=x+ox[i],ny=y+oy[i];
                if (nx>=0 && ny>=0 && nx<64 && ny<64 && map->planes[0][ny*64+nx]>=107) raw=map->planes[0][ny*64+nx];
            }
        }
        areaMap[x][y]=raw>=107 ? (int)raw-107 : 0;
    }
    for (unsigned i=0;i<doornum;++i) {
        const doorobj_t *d=&doorobjlist[i];
        areaMap[d->x][d->y]=areaMap[d->x-(d->vertical?1U:0U)][d->y-(d->vertical?0U:1U)];
    }
    for (int y=0;y<64;++y) for (int x=0;x<64;++x) {
        unsigned tile=map->planes[1][y*64+x];
        if (tile>=23 && tile<=70 && !SpawnStatic(x,y,tile-23)) return false;
        if (tile==124) { if (!SpawnNewObj(x,y,&s_grddie4,inertobj,0,false,true)) return false;continue; }
        if (tile==98) {
            if (!tilemap[x][y] || tilemap[x][y]>=64) return CA_Fail("Invalid pushwall marker");
            pushable[x][y]=true;++gamestate.secrettotal;continue;
        }
        if (tile==214 || tile==197 || tile==196 || tile==215 || tile==179 ||
            tile==160 || tile==178 || (tile>=224 && tile<=227)) {
            if (!SpawnSpecial(x,y,tile)) return false;
            continue;
        }
        /* EXITTILE is a movement trigger, not an actor to allocate at startup. */
        if (tile==99) {
            if (x<1 || y<1 || x>62 || y>61 || (tilemap[x][y] && tilemap[x][y]!=64))
                return CA_Fail("Invalid victory tile");
            continue;
        }
        /* WL_GAME.C ScanInfoPlane: two 36-code normal-enemy bands;
           mutants have their own two 18-code bands. Exclude boss codes. */
        if (tile>=180 && tile<=213) { if (gamestate.difficulty<3) continue;tile-=36; }
        if (tile>=144 && tile<=177) { if (gamestate.difficulty<2) continue;tile-=36; }
        if (tile>=252 && tile<=259) { if (gamestate.difficulty<3) continue;tile-=18; }
        if (tile>=234 && tile<=241) { if (gamestate.difficulty<2) continue;tile-=18; }
        statetype *state=NULL;int kind=guardobj,dir=0;bool patrol=false;
        if (tile>=108 && tile<=115) { dir=(int)(tile-108)%4;patrol=tile>=112;state=patrol?&s_grdpath1:&s_grdstand; }
        else if (tile>=116 && tile<=123) { kind=officerobj;dir=(int)(tile-116)%4;patrol=tile>=120;state=patrol?&s_ofcpath1:&s_ofcstand; }
        else if (tile>=126 && tile<=133) { kind=ssobj;dir=(int)(tile-126)%4;patrol=tile>=130;state=patrol?&s_sspath1:&s_ssstand; }
        else if (tile>=138 && tile<=141) { kind=dogobj;dir=(int)tile-138;patrol=true;state=&s_dogpath1; }
        else if (tile>=216 && tile<=223) { kind=mutantobj;dir=(int)(tile-216)%4;patrol=tile>=220;state=patrol?&s_mutpath1:&s_mutstand; }
        /* Original SpawnStand has no dog case; these unused codes are unsafe. */
        else if (tile>=134 && tile<=137) return CA_Fail("Unsupported stationary dog spawn %u",tile);
        if (state && !SpawnNewObj(x,y,state,kind,dir,patrol,false)) return false;
    }
    ConnectAreas();return true;
}
/* WL_ACT2.C A_DeathScream, registered (non-UPLOAD), non-SPEAR version.
   The easter-egg draw uses ID_CA's absolute mapon, not episode-local mapon. */
void A_DeathScream(objtype *ob)
{
    if (gamestate.episode*10+gamestate.mapon==9 && !US_RndT()) {
        switch (ob->obclass) {
        case mutantobj:case guardobj:case officerobj:case ssobj:case dogobj:
            GameSound(DEATHSCREAM6SND,ob);return;
        }
    }
    switch (ob->obclass) {
    case guardobj: {
        static const soundnames sounds[]={DEATHSCREAM1SND,DEATHSCREAM2SND,
            DEATHSCREAM3SND,DEATHSCREAM4SND,DEATHSCREAM5SND,DEATHSCREAM7SND,
            DEATHSCREAM8SND,DEATHSCREAM9SND};
        GameSound(sounds[US_RndT()%8],ob);break;
    }
    case mutantobj:GameSound(AHHHGSND,ob);break;
    case officerobj:GameSound(NEINSOVASSND,ob);break;
    case ssobj:GameSound(LEBENSND,ob);break;
    case dogobj:GameSound(DOGDEATHSND,ob);break;
    case bossobj:GameSound(MUTTISND,NULL);break;
    case gretelobj:GameSound(MEINSND,NULL);break;
    case schabbobj:GameSound(MEINGOTTSND,NULL);break;
    case giftobj:GameSound(DONNERSND,NULL);break;
    case fatobj:GameSound(ROSESND,NULL);break;
    case fakeobj:GameSound(HITLERHASND,NULL);break;
    case mechahitlerobj:GameSound(SCHEISTSND,NULL);break;
    case realhitlerobj:GameSound(EVASND,NULL);break;
    }
}

bool Save_ActorAllocated(unsigned slot) { return slot<MAXACTORS && allocated[slot]; }
void Save_SetActorAllocation(const bool used[MAXACTORS])
{
    memcpy(allocated,used,sizeof(allocated));allocationfailed=false;soundpositioned=false;
}
