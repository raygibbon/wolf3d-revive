/* SPDX-License-Identifier: GPL-2.0-only */
/* Native counterpart of WL_MAIN.C SaveTheGame/LoadTheGame. Version 1 is
   explicit little-endian fields, not the DOS structure/pointer dump. */
#include "WL_SAVE.h"
#include "ID_CA.h"
#include "ID_FILE.h"
#include "ID_PM.h"
#include "ID_SD.h"
#include "ID_US_1.h"
#include "ID_IN.h"
#include "platform/data_file.h"
#include "platform/save_file.h"
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#define SAVE_HEADER 96U
#define SAVE_LIMIT (256U*1024U)
#define SAVE_STATE(name) extern statetype name;
#include "WL_SAVE_STATES.inc"
#undef SAVE_STATE
static statetype *const states[]={NULL,
#define SAVE_STATE(name) &name,
#include "WL_SAVE_STATES.inc"
#undef SAVE_STATE
};
unsigned Save_StateCount(void) { return (unsigned)(sizeof(states)/sizeof(states[0])); }
statetype *Save_State(unsigned id) { return id<Save_StateCount()?states[id]:NULL; }
unsigned Save_StateID(const statetype *state)
{ for(unsigned i=0;i<Save_StateCount();++i) if(states[i]==state) return i;return UINT32_MAX; }
static char directory[4096];
const char *Save_Directory(void) { return directory; }
bool Save_SetDirectory(const char *path)
{
    if(!path || !*path || strlen(path)>=sizeof(directory)-32) return CA_Fail("Save directory unavailable/too long");
    (void)strcpy(directory,path);return true;
}
static bool Path(unsigned slot,char result[4096])
{
    if(slot>=SAVE_SLOTS || !*directory) return CA_Fail("Save storage unavailable or invalid slot");
    int n=snprintf(result,4096,"%s/slot%u.w3s",directory,slot+1);
    return n>0 && n<4096;
}
static uint32_t CRC(uint32_t crc,const void *data,size_t size)
{
    const uint8_t *p=data;
    while(size--) {
        crc^=*p++;
        for(unsigned i=0;i<8;++i) crc=(crc>>1)^((crc&1)?UINT32_C(0xedb88320):0);
    }
    return crc;
}
static void Put32(uint8_t *p,uint32_t v) { for(unsigned i=0;i<4;++i) p[i]=(uint8_t)(v>>(i*8)); }
static bool Identity(uint32_t ids[3])
{
    static const char *const names[]={"MAPHEAD.WL6","GAMEMAPS.WL6","VSWAP.WL6"};
    for(unsigned i=0;i<3;++i) {
        FILE *file=CA_OpenDataFile(CA_DataDirectory(),names[i]);
        if(!file) return CA_Fail("Cannot identify save resources: %s",names[i]);
        uint8_t buffer[8192];size_t n,total=0;uint32_t crc=UINT32_MAX;
        while((n=fread(buffer,1,sizeof(buffer),file))!=0) { total+=n;if(total>32U*1024U*1024U) break;crc=CRC(crc,buffer,n); }
        bool ok=!ferror(file) && total && total<=32U*1024U*1024U;(void)fclose(file);
        if(!ok) return CA_Fail("Cannot read save resource identity");
        ids[i]=~crc;
    }
    return true;
}
typedef struct {
    gametype game;levelratio_t ratios[8];objtype player,actors[MAXACTORS];
    bool used[MAXACTORS];unsigned count,stat_count,door_count,rng,mode;
    unsigned state[MAXACTORS+1];int next[MAXACTORS+1],at[64][64];
    statobj_t stats[MAXSTATS];doorobj_t doors[64];
    uint8_t walls[64][64];int areas[64][64];bool solid[64][64],push[64][64],noise;
    unsigned wall_state,wall_pos;int wall_x,wall_y,wall_dir,thrust;
    save_controls controls;maptype map;
} snapshot;
typedef struct { uint8_t *data;size_t pos,size;bool read,ok; } codec;
static uint32_t Word(codec *c,uint32_t v)
{
    if(!c->ok || c->pos>c->size || c->size-c->pos<4) { c->ok=false;return 0; }
    if(c->read) v=CA_LE32(c->data+c->pos);else Put32(c->data+c->pos,v);
    c->pos+=4;return v;
}
static void Integer(codec *c,int *v)
{
    uint32_t n=Word(c,(uint32_t)*v);
    if(c->read) *v=n<=INT32_MAX?(int)n:-(int)(UINT32_MAX-n)-1;
}
static void Unsigned(codec *c,unsigned *v) { *v=(unsigned)Word(c,(uint32_t)*v); }
static void Boolean(codec *c,bool *v) { uint32_t n=Word(c,*v?1:0);if(n>1) c->ok=false;*v=n!=0; }
#define GAME_INTS(X) X(health) X(ammo) X(lives) X(keys) X(score) X(nextextra) X(weapon) X(bestweapon) X(chosenweapon) X(attackframe) X(weaponframe) X(attackcount) X(faceframe) X(killcount) X(killtotal) X(treasurecount) X(treasuretotal) X(episode) X(mapon) X(difficulty) X(oldscore) X(secretcount) X(secrettotal) X(killx) X(killy)
#define ACTOR_INTS(X) X(x) X(y) X(angle) X(tilex) X(tiley) X(dir) X(obclass) X(areanumber) X(hitpoints) X(ticcount) X(temp1) X(temp2) X(viewx) X(viewheight) X(speed) X(distance) X(transx) X(transy)
static void Actor(codec *c,objtype *a)
{
#define FIELD(n) Integer(c,&a->n);
    ACTOR_INTS(FIELD)
#undef FIELD
    Unsigned(c,&a->flags);Boolean(c,&a->active);
}
static void Ratio(codec *c,levelratio_t *r)
{
    Unsigned(c,&r->kill);Unsigned(c,&r->secret);Unsigned(c,&r->treasure);Unsigned(c,&r->seconds);Unsigned(c,&r->par);Integer(c,&r->bonus);
}
static void Payload(codec *c,snapshot *s)
{
#define FIELD(n) Integer(c,&s->game.n);
    GAME_INTS(FIELD)
#undef FIELD
    s->game.TimeCount=Word(c,s->game.TimeCount);Boolean(c,&s->game.died);Boolean(c,&s->game.victoryflag);
    for(unsigned i=0;i<8;++i) Ratio(c,&s->ratios[i]);
    Unsigned(c,&s->rng);Unsigned(c,&s->mode);Unsigned(c,&s->count);Unsigned(c,&s->stat_count);Unsigned(c,&s->door_count);
    if(s->count>MAXACTORS || s->stat_count>MAXSTATS || s->door_count>64) { c->ok=false;return; }
    Actor(c,&s->player);Unsigned(c,&s->state[0]);Integer(c,&s->next[0]);
    for(unsigned i=0;i<s->count;++i) {
        Boolean(c,&s->used[i]);
        if(s->used[i]) { Actor(c,&s->actors[i]);Unsigned(c,&s->state[i+1]);Integer(c,&s->next[i+1]); }
    }
    for(unsigned i=0;i<s->stat_count;++i) {
        statobj_t *o=&s->stats[i];Integer(c,&o->tilex);Integer(c,&o->tiley);Integer(c,&o->shapenum);
        unsigned item=(unsigned)o->itemnumber;Unsigned(c,&item);o->itemnumber=(stat_t)item;Boolean(c,&o->bonus);
    }
    for(unsigned i=0;i<s->door_count;++i) {
        doorobj_t *d=&s->doors[i];Unsigned(c,&d->x);Unsigned(c,&d->y);Unsigned(c,&d->lock);Boolean(c,&d->vertical);
        unsigned action=(unsigned)d->action,position=d->position;Unsigned(c,&action);Unsigned(c,&position);
        if(action>3 || position>65535) c->ok=false;
        d->action=(dooraction)action;d->position=(uint16_t)position;Unsigned(c,&d->ticcount);
    }
    Unsigned(c,&s->wall_state);Unsigned(c,&s->wall_pos);Integer(c,&s->wall_x);Integer(c,&s->wall_y);Integer(c,&s->wall_dir);
    for(unsigned x=0;x<64;++x) for(unsigned y=0;y<64;++y) {
        unsigned tile=s->walls[x][y];Unsigned(c,&tile);if(tile>255) c->ok=false;s->walls[x][y]=(uint8_t)tile;
        Integer(c,&s->areas[x][y]);Integer(c,&s->at[x][y]);Boolean(c,&s->solid[x][y]);Boolean(c,&s->push[x][y]);
    }
    Boolean(c,&s->noise);Integer(c,&s->thrust);Integer(c,&s->controls.facecount);Integer(c,&s->controls.anglefrac);
    Boolean(c,&s->controls.attackheld);Boolean(c,&s->controls.gotgatgun);
}
static int Reference(const objtype *o)
{
    if(!o) return -1;
    for(unsigned i=0;i<MAXACTORS;++i) if(o==&objlist[i]) return (int)i;
    return -2;
}
static void Capture(snapshot *s)
{
    s->game=gamestate;memcpy(s->ratios,LevelRatios,sizeof(s->ratios));s->player=player;
    s->count=objcount;s->stat_count=statcount;s->door_count=doornum;s->rng=US_RndIndex();s->mode=SD_DigitizedMode()?1U:0U;
    s->state[0]=Save_StateID(player.state);s->next[0]=Reference(player.next);
    for(unsigned i=0;i<objcount;++i) if((s->used[i]=Save_ActorAllocated(i))) {
        s->actors[i]=objlist[i];s->state[i+1]=Save_StateID(objlist[i].state);s->next[i+1]=Reference(objlist[i].next);
    }
    memcpy(s->stats,statobjlist,sizeof(s->stats));memcpy(s->doors,doorobjlist,sizeof(s->doors));
    memcpy(s->walls,tilemap,sizeof(s->walls));memcpy(s->areas,areaMap,sizeof(s->areas));
    memcpy(s->solid,staticblock,sizeof(s->solid));memcpy(s->push,pushable,sizeof(s->push));
    for(unsigned x=0;x<64;++x) for(unsigned y=0;y<64;++y) s->at[x][y]=Reference(actorat[x][y]);
    s->wall_state=pwallstate;s->wall_pos=pwallpos;s->wall_x=pwallx;s->wall_y=pwally;s->wall_dir=pwalldir;
    s->noise=madenoise;s->thrust=thrustspeed;Save_GetCombat(&s->controls);s->controls.anglefrac=Save_GetAngleFraction();
}
static bool Coordinate(int v) { return v>=0 && v<64; }
static bool Position(int v) { return v>=0 && v<64*TILEGLOBAL; }
static bool ValidActor(const objtype *o,bool isplayer)
{
    return Coordinate(o->tilex) && Coordinate(o->tiley) && Position(o->x) && Position(o->y) &&
        o->angle>=0 && o->angle<360 && o->dir>=0 && o->dir<=nodir && o->obclass>=0 && o->obclass<=rocketobj &&
        (isplayer || o->obclass>=inertobj) && o->areanumber>=0 && o->areanumber<37 && !(o->flags&~253U) &&
        o->speed>=0 && o->speed<=65536 && o->distance>=-65536 && o->distance<=64*TILEGLOBAL &&
        o->hitpoints>=-100000 && o->hitpoints<=100000 && o->ticcount>=-10000 && o->ticcount<=10000 &&
        o->temp1>=-65536 && o->temp1<=1000000 && o->temp2>=-65536 && o->temp2<=1000000 &&
        o->viewx>=-1000000 && o->viewx<=1000000 && o->viewheight>=0 && o->viewheight<=1000000 &&
        o->transx>=-64*TILEGLOBAL && o->transx<=64*TILEGLOBAL && o->transy>=-64*TILEGLOBAL && o->transy<=64*TILEGLOBAL;
}
/* Match SetupGameLevel's reachable-space enclosure rule. Unused floor at an
   outer edge exists in original maps, so requiring every edge cell to be a
   wall would reject legitimate data. Closed doors remain traversable here. */
static bool ValidMap(const snapshot *s)
{
    if(s->player.tilex!=s->player.x/TILEGLOBAL || s->player.tiley!=s->player.y/TILEGLOBAL ||
       s->player.x<MINDIST || s->player.y<MINDIST || s->player.x>=64*TILEGLOBAL-MINDIST || s->player.y>=64*TILEGLOBAL-MINDIST) return false;
    unsigned tile=s->walls[s->player.tilex][s->player.tiley];
    if(tile && tile!=64 && (tile&0xc0)!=0x80) return false;
    bool seen[64][64]={{false}};unsigned queue[4096],head=0,tail=0;
    queue[tail++]=(unsigned)(s->player.tiley*64+s->player.tilex);seen[s->player.tilex][s->player.tiley]=true;
    static const int dx[]={1,0,-1,0},dy[]={0,1,0,-1};
    while(head<tail) {
        unsigned cell=queue[head++];int x=(int)(cell%64),y=(int)(cell/64);
        if(!x || !y || x==63 || y==63) return false;
        for(unsigned i=0;i<4;++i) {
            int nx=x+dx[i],ny=y+dy[i];tile=s->walls[nx][ny];
            if(seen[nx][ny] || (tile && tile!=64 && (tile&0xc0)!=0x80)) continue;
            seen[nx][ny]=true;queue[tail++]=(unsigned)(ny*64+nx);
        }
    }
    return true;
}
static bool Valid(const snapshot *s)
{
    const gametype *g=&s->game;
    if(g->episode<0 || g->episode>=6 || g->mapon<0 || g->mapon>=10 || g->difficulty<0 || g->difficulty>=4 ||
       g->health<=0 || g->health>100 || g->ammo<0 || g->ammo>99 || g->lives<0 || g->lives>9 || g->keys<0 || g->keys>15 ||
       g->weapon<0 || g->weapon>3 || g->bestweapon<g->weapon || g->bestweapon>3 || g->chosenweapon<0 || g->chosenweapon>g->bestweapon ||
       g->attackframe<0 || g->attackframe>4 || g->weaponframe<0 || g->weaponframe>4 || g->attackcount<0 || g->attackcount>12 ||
       g->faceframe<0 || g->faceframe>2 || g->score<0 || g->score>100000000 || g->nextextra<=g->score || g->nextextra>100040000 ||
       g->oldscore<0 || g->oldscore>100000000 || g->died || g->victoryflag ||
       !Position(g->killx) || !Position(g->killy) || s->rng>255 || s->mode>1 ||
       s->count>MAXACTORS || s->stat_count>MAXSTATS || s->door_count>64 ||
       s->controls.facecount<0 || s->controls.facecount>256 || s->controls.anglefrac<=-20 || s->controls.anglefrac>=20 ||
       s->thrust<0 || s->thrust>100000 || !ValidActor(&s->player,true) || s->state[0]!=0) return false;
    int counts[]={g->killcount,g->killtotal,g->secretcount,g->secrettotal,g->treasurecount,g->treasuretotal};
    for(unsigned i=0;i<6;++i) if(counts[i]<0 || counts[i]>4096) return false;
    for(unsigned i=0;i<8;++i) {
        const levelratio_t *r=&s->ratios[i];
        if(r->kill>10000 || r->secret>10000 || r->treasure>10000 || r->seconds>5940 || r->par>10000 || r->bonus<0 || r->bonus>10000000) return false;
    }
    bool visited[MAXACTORS]={false};int next=s->next[0];
    while(next!=-1) {
        if(next<0 || (unsigned)next>=s->count || !s->used[next] || visited[next]) return false;
        visited[next]=true;next=s->next[next+1];
    }
    for(unsigned i=0;i<s->count;++i) {
        if(s->used[i]!=visited[i]) return false;
        if(s->used[i] && (!ValidActor(&s->actors[i],false) || !s->state[i+1] || s->state[i+1]>=Save_StateCount() ||
           (s->actors[i].distance<0 && (unsigned)(-s->actors[i].distance-1)>=s->door_count))) return false;
    }
    if(s->wall_state>255 || s->wall_pos>63) return false;
    if(s->wall_state && (!Coordinate(s->wall_x) || !Coordinate(s->wall_y) || s->wall_x==0 || s->wall_x==63 ||
       s->wall_y==0 || s->wall_y==63 || s->wall_dir<0 || s->wall_dir>6 || (s->wall_dir&1))) return false;
    for(unsigned x=0;x<64;++x) for(unsigned y=0;y<64;++y) {
        unsigned tile=s->walls[x][y];int ref=s->at[x][y];
        if(s->areas[x][y]<0 || s->areas[x][y]>=37 || ref< -1 || (ref>=0 && ((unsigned)ref>=s->count || !s->used[ref]))) return false;
        if((tile&0xc0)==0x80 && (tile&63)>=s->door_count) return false;
        if((tile&0xc0)==0xc0 && (!s->wall_state || x!=(unsigned)s->wall_x || y!=(unsigned)s->wall_y || !(tile&63))) return false;
    }
    for(unsigned i=0;i<s->door_count;++i) {
        const doorobj_t *d=&s->doors[i];
        if(d->x<1 || d->x>62 || d->y<1 || d->y>62 || d->lock>5 || (unsigned)d->action>3 || d->ticcount>300 ||
           s->walls[d->x][d->y]!=(128U+i)) return false;
    }
    for(unsigned i=0;i<s->stat_count;++i) {
        const statobj_t *o=&s->stats[i];
        if(!Coordinate(o->tilex) || !Coordinate(o->tiley) || o->shapenum< -1 || o->shapenum>4095 || (unsigned)o->itemnumber>bo_spear) return false;
    }
    return ValidMap(s);
}
static bool Description(const char *p)
{
    if(!p || !*p) return false;
    for(unsigned i=0;i<SAVE_DESCRIPTION;++i) { if(!p[i]) return true;if((unsigned char)p[i]<32 || (unsigned char)p[i]>126) return false; }
    return false;
}
static save_status Header(const uint8_t *h,size_t file_size,unsigned slot,save_info *info)
{
    if(memcmp(h,"W3RSAVE",8)) return SAVE_CORRUPT;
    info->version=CA_LE32(h+8);
    if(info->version!=SAVE_VERSION) return SAVE_UNSUPPORTED;
    if(CA_LE32(h+12)!=SAVE_HEADER || CA_LE32(h+16)>SAVE_LIMIT ||
       SAVE_HEADER+(size_t)CA_LE32(h+16)!=file_size || CA_LE32(h+20)!=slot ||
       CA_LE32(h+28)>5 || CA_LE32(h+32)>9 || CA_LE32(h+36)>3 || !Description((const char *)h+40)) return SAVE_CORRUPT;
    memcpy(info->description,h+40,SAVE_DESCRIPTION);info->episode=CA_LE32(h+28);info->level=CA_LE32(h+32);info->difficulty=CA_LE32(h+36);
    return SAVE_VALID;
}
/* Metadata inspection streams the integrity check, without deserializing or
   changing the game. Full loads retain the bounded payload for staging. */
static uint8_t *Read(unsigned slot,save_info *info,bool payload)
{
    char path[4096];memset(info,0,sizeof(*info));info->status=SAVE_UNAVAILABLE;
    if(!Path(slot,path)) return NULL;
    FILE *file=SaveFile_Open(path);
    if(!file) { info->status=SaveFile_Missing(path)?SAVE_EMPTY:SAVE_UNAVAILABLE;return NULL; }
    uint8_t h[SAVE_HEADER],buffer[4096],*data=NULL;long size;
    info->status=SAVE_CORRUPT;
    if(fseek(file,0,SEEK_END) || (size=ftell(file))<(long)SAVE_HEADER ||
       fseek(file,0,SEEK_SET) || fread(h,1,sizeof(h),file)!=sizeof(h)) goto done;
    info->status=Header(h,(size_t)size,slot,info);if(info->status!=SAVE_VALID) goto done;
    size_t left=CA_LE32(h+16);uint32_t expected=CA_LE32(h+24);Put32(h+24,0);
    uint32_t crc=CRC(UINT32_MAX,h,sizeof(h));
    if(payload) { data=malloc((size_t)size);if(!data) { info->status=SAVE_UNAVAILABLE;goto done; }memcpy(data,h,sizeof(h)); }
    size_t at=SAVE_HEADER;
    while(left) {
        size_t n=left>sizeof(buffer)?sizeof(buffer):left;
        if(fread(buffer,1,n,file)!=n) { info->status=SAVE_CORRUPT;break; }
        crc=CRC(crc,buffer,n);if(data) memcpy(data+at,buffer,n);at+=n;left-=n;
    }
    if(~crc!=expected || left) info->status=SAVE_CORRUPT;
done:
    (void)fclose(file);
    if(info->status!=SAVE_VALID) { free(data);data=NULL; }
    return data;
}
void Save_ReadInfo(unsigned slot,save_info *info) { if(info) (void)Read(slot,info,false); }
bool Save_Write(unsigned slot,const char *description,bool overwrite)
{
    if(!GameCanSave()) return CA_Fail("Saving is unavailable during death, intermissions or victory");
    if(!Description(description)) return CA_Fail("Save description must contain 1-31 printable characters");
    char path[4096];if(!Path(slot,path)) return false;
    snapshot *s=calloc(1,sizeof(*s));uint8_t *data=calloc(1,SAVE_HEADER+SAVE_LIMIT);
    if(!s || !data) { free(s);free(data);return CA_Fail("Cannot allocate save buffer"); }
    Capture(s);bool ok=Valid(s);
    if(!ok) (void)CA_Fail("Current game cannot be represented by save version 1");
    codec c={data+SAVE_HEADER,0,SAVE_LIMIT,false,true};if(ok) Payload(&c,s);
    if(ok) ok=c.ok;
    if(ok) {
        memcpy(data,"W3RSAVE",8);Put32(data+8,SAVE_VERSION);Put32(data+12,SAVE_HEADER);Put32(data+16,(uint32_t)c.pos);Put32(data+20,slot);
        Put32(data+28,(unsigned)s->game.episode);Put32(data+32,(unsigned)s->game.mapon);Put32(data+36,(unsigned)s->game.difficulty);
        memcpy(data+40,description,strlen(description));uint32_t ids[3];ok=Identity(ids);
        if(ok) {
            for(unsigned i=0;i<3;++i) Put32(data+72+i*4,ids[i]);
            Put32(data+84,s->mode);
            Put32(data+24,~CRC(UINT32_MAX,data,SAVE_HEADER+c.pos));
            ok=SaveFile_WriteAtomic(path,data,SAVE_HEADER+c.pos,overwrite);
            if(!ok) (void)CA_Fail("Cannot replace save slot %u; previous save retained",slot+1);
        }
    }
    free(data);free(s);return ok;
}
static bool Resources(snapshot *s)
{
    if(!CA_ReadMap((unsigned)(s->game.episode*10+s->game.mapon),&s->map)) return false;
    if(!PM_Ready() && !PM_Startup()) return false;
    uint8_t pixels[4096],mask[4096];
    for(unsigned i=0;i<s->count;++i) if(s->used[i]) {
        const statetype *state=Save_State(s->state[i+1]);
        if(state->shapenum && !PM_DecodeSprite((unsigned)state->shapenum,pixels,mask)) return false;
    }
    for(unsigned i=0;i<s->stat_count;++i) if(s->stats[i].shapenum>=0 && !PM_DecodeSprite((unsigned)s->stats[i].shapenum,pixels,mask)) return false;
    for(unsigned x=0;x<64;++x) for(unsigned y=0;y<64;++y) {
        unsigned tile=s->walls[x][y];if(!tile || (tile&128)) continue;tile&=63;
        if(tile && (tile*2-1>=PMSpriteStart || !PM_GetPage((tile-1)*2,NULL) || !PM_GetPage((tile-1)*2+1,NULL))) return false;
    }
    return true;
}
static objtype *Object(int ref) { return ref<0?NULL:&objlist[ref]; }
static void Apply(const snapshot *s,uint64_t ticks)
{
    gamestate=s->game;player=s->player;player.state=Save_State(s->state[0]);player.next=Object(s->next[0]);
    memset(objlist,0,sizeof(objlist));
    for(unsigned i=0;i<s->count;++i) if(s->used[i]) {
        objlist[i]=s->actors[i];objlist[i].state=Save_State(s->state[i+1]);objlist[i].next=Object(s->next[i+1]);
    }
    objcount=s->count;statcount=s->stat_count;doornum=s->door_count;Save_SetActorAllocation(s->used);
    memcpy(statobjlist,s->stats,sizeof(s->stats));memcpy(doorobjlist,s->doors,sizeof(s->doors));
    memcpy(tilemap,s->walls,sizeof(s->walls));memcpy(areaMap,s->areas,sizeof(s->areas));
    memcpy(staticblock,s->solid,sizeof(s->solid));memcpy(pushable,s->push,sizeof(s->push));
    for(unsigned x=0;x<64;++x) for(unsigned y=0;y<64;++y) actorat[x][y]=Object(s->at[x][y]);
    memcpy(LevelRatios,s->ratios,sizeof(s->ratios));pwallstate=s->wall_state;pwallpos=s->wall_pos;pwallx=s->wall_x;pwally=s->wall_y;pwalldir=s->wall_dir;
    thrustspeed=s->thrust;madenoise=s->noise;playstate=ex_playing;BuildTables();ConnectAreas();
    GameRestoreClock(ticks);Save_SetCombat(&s->controls);Save_SetAngleFraction(s->controls.anglefrac);US_InitRndT(s->rng);IN_ClearKeysDown();
    SD_SetDigitizedMode(s->mode!=0);SD_StopSound();SD_StartMusic(SD_LevelMusic((unsigned)gamestate.episode,(unsigned)gamestate.mapon));SD_FlushOutput();
    s_schabbdie2.tictime=s_giftdie2.tictime=s_fatdie2.tictime=s_hitlerdie2.tictime=s->mode?140:5;
}
bool Save_Load(unsigned slot,uint64_t ticks)
{
    save_info info;uint8_t *data=Read(slot,&info,true);
    if(!data) return CA_Fail("Cannot load slot %u: %s",slot+1,info.status==SAVE_EMPTY?"empty":info.status==SAVE_UNSUPPORTED?"unsupported save version":info.status==SAVE_UNAVAILABLE?"storage unavailable":"corrupt save");
    snapshot *s=calloc(1,sizeof(*s));if(!s) { free(data);return CA_Fail("Cannot allocate load staging area"); }
    codec c={data+SAVE_HEADER,0,CA_LE32(data+16),true,true};Payload(&c,s);
    bool ok=c.ok && c.pos==c.size && Valid(s) && (unsigned)s->game.episode==info.episode &&
        (unsigned)s->game.mapon==info.level && (unsigned)s->game.difficulty==info.difficulty && s->mode==CA_LE32(data+84);
    if(!ok) (void)CA_Fail("Invalid save state, coordinates or references");
    uint32_t ids[3];
    if(ok) { ok=Identity(ids);if(ok) for(unsigned i=0;i<3;++i) if(ids[i]!=CA_LE32(data+72+i*4)) { ok=CA_Fail("Save belongs to different original game resources");break; } }
    if(ok) ok=Resources(s);
    /* Last fallible operation. No active game fields change before this point. */
    if(ok) ok=CA_InstallMap(&s->map);
    if(ok) Apply(s,ticks);
    free(s);free(data);return ok;
}
