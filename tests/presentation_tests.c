/* SPDX-License-Identifier: GPL-2.0-only */
#include "engine/WL_PRESENT.h"
#include "engine/WL_RENDER.h"
#include "engine/WL_SAVE.h"
#include "engine/WL_CONTROLS.h"
#include "engine/ID_CA.h"
#include "engine/ID_VL.h"
#include "engine/ID_US_1.h"
#include <SDL3/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#define CHECK(x) do { if(!(x)) { fprintf(stderr,"%d: %s (%s)\n",__LINE__,#x,CA_Error());exit(1); } } while(0)
static statetype teststand={0,SPR_GRD_S_1,0,NULL,NULL,NULL};
static uint32_t Hash(const void *p,size_t n) { const unsigned char *s=p;uint32_t h=2166136261U;while(n--) h=(h^*s++)*16777619U;return h; }
static uint32_t State(void)
{
    return Hash(&gamestate,sizeof(gamestate))^Hash(&player,sizeof(player))^Hash(objlist,sizeof(objlist))^
        Hash(tilemap,sizeof(tilemap))^Hash(spotvis,sizeof(spotvis))^Hash(doorobjlist,sizeof(doorobjlist))^
        Hash(framebuffer,VL_PIXELS)^US_RndIndex();
}
static uint64_t Half(unsigned tick) { return WL_TickNS(tick)+(WL_TickNS(tick)-WL_TickNS(tick-1))/2; }
static void Snapshots(void)
{
    CHECK(GameStart(0));player.angle=359;
    objtype *actor=SpawnActor(3,3,&teststand);CHECK(actor);unsigned slot=(unsigned)(actor-objlist);
    doornum=1;doorobjlist[0].position=1000;
    WL_PresentReset();WL_PresentCapture(100);
    int32_t x=player.x,ax=actor->x;uint64_t identity=WL_PresentCurrent()->actors[slot].identity;
    player.x+=4096;player.angle=1;actor->x+=4096;doorobjlist[0].position=3000;++gamestate.TimeCount;WL_PresentCapture(101);
    wl_present_snapshot *sample=malloc(sizeof(*sample));CHECK(sample);
    uint32_t immutable=Hash(WL_PresentCurrent(),sizeof(*sample));
    CHECK(WL_PresentSample(Half(101),sample));
    CHECK(abs(sample->camera.x-x-2048)<=1 && (sample->camera.angle<0.00001 || sample->camera.angle>359.99999));
    CHECK(abs(sample->actors[slot].x-ax-2048)<=1 && abs((int)sample->doors[0].position-2000)<=1);
    CHECK(Hash(WL_PresentCurrent(),sizeof(*sample))==immutable);
    RemoveActor(actor);objtype *replacement=GetNewActor();CHECK(replacement==actor);replacement->state=&teststand;
    replacement->x=ax+32768;replacement->y=3*TILEGLOBAL;replacement->dir=0;
    ++gamestate.TimeCount;WL_PresentCapture(102);CHECK(WL_PresentSample(Half(102),sample));
    CHECK(sample->actors[slot].identity!=identity && sample->actors[slot].x==replacement->x);
    pwallstate=127;pwallpos=63;pwallx=4;pwally=4;pwalldir=0;tilemap[4][4]=0xc1;tilemap[5][4]=1;
    WL_PresentCapture(103);pwallstate=128;pwallpos=0;pwallx=5;tilemap[4][4]=0;tilemap[5][4]=0xc1;
    WL_PresentCapture(104);CHECK(WL_PresentSample(Half(104),sample));
    CHECK(sample->push_x==4 && sample->tiles[4][4]==0xc1 && abs(sample->push_offset-65024)<=1);
    CHECK(WL_PresentSample(WL_TickNS(105),sample) && sample->push_offset==0 && sample->tiles[5][4]==0xc1);
    player.x+=3*TILEGLOBAL;WL_PresentCapture(105);CHECK(WL_PresentSample(Half(105),sample) && sample->camera.x==player.x);
    GamePauseClock(200);CHECK(WL_PresentSample(WL_TickNS(200),sample) && sample->camera.x==player.x);
    CHECK(GameStart(0) && Save_Write(5,"snapshot reset",true));
    CT_Key(SDL_SCANCODE_UP,true);CHECK(GameStep(10));CT_Key(SDL_SCANCODE_UP,false);
    CHECK(Save_Load(5,300) && WL_PresentSample(WL_TickNS(300),sample));CHECK(sample->camera.x==player.x && sample->camera.y==player.y);
    CHECK(GameStartMap(0,1,2,0));CHECK(WL_PresentSample(0,sample) && sample->level==1 && sample->camera.x==player.x);
    CHECK(GameStart(0));player.angle=0;objtype attacker=player;attacker.y-=TILEGLOBAL;TakeDamage(1000,&attacker);
    CHECK(GameStep(1) && GameStep(2));CHECK(WL_PresentSample(Half(2),sample));CHECK(fabs(sample->camera.angle-3)<0.00001);
    uint64_t death_tick=2;
    while(gamestate.died && death_tick<600) CHECK(GameStep(++death_tick));
    CHECK(!gamestate.died && WL_PresentSample(WL_TickNS(death_tick)+1000000,sample));
    CHECK(sample->camera.x==player.x && sample->camera.y==player.y && sample->camera.angle==player.angle);
    CHECK(GameStart(0));WL_PresentCapture(390);player.x+=4096;CHECK(NewViewSize(16));
    WL_PresentCapture(391);CHECK(WL_PresentSample(Half(391),sample) && sample->camera.x==player.x);
    CHECK(NewViewSize(15));
    /* Unmatched doors must snap, even when an index was reused. */
    CHECK(GameStart(0));doornum=1;doorobjlist[0]=(doorobj_t){4,4,0,true,dr_opening,1000,0};
    WL_PresentReset();WL_PresentCapture(400);
    doornum=2;doorobjlist[1]=(doorobj_t){5,4,0,true,dr_opening,3000,0};
    doorobjlist[0].x=6;doorobjlist[0].position=5000;WL_PresentCapture(401);
    CHECK(WL_PresentSample(Half(401),sample));
    CHECK(sample->doors[0].position==5000 && sample->doors[1].position==3000);
    /* Only pushwall cells rewind; a door opening in this tick stays floor. */
    tilemap[8][8]=128;pwallstate=127;pwallpos=63;pwallx=4;pwally=4;pwalldir=0;
    tilemap[4][4]=0xc1;tilemap[5][4]=1;tilemap[6][4]=0;WL_PresentCapture(402);
    tilemap[8][8]=0;tilemap[6][4]=1;pwallstate=128;pwallpos=0;pwallx=5;tilemap[4][4]=0;tilemap[5][4]=0xc1;
    WL_PresentCapture(403);CHECK(WL_PresentSample(Half(403),sample));
    CHECK(sample->tiles[8][8]==0 && sample->tiles[4][4]==0xc1 && sample->tiles[6][4]==0);
    /* Unrelated pushwalls and actor teleports never connect across space. */
    pwallx=10;pwallpos=10;WL_PresentCapture(404);CHECK(WL_PresentSample(Half(404),sample));
    CHECK(sample->push_x==10 && sample->push_offset==10240);
    actor=SpawnActor(3,3,&teststand);CHECK(actor);slot=(unsigned)(actor-objlist);
    actor->dir=7;WL_PresentCapture(405);actor->dir=0;WL_PresentCapture(406);
    CHECK(WL_PresentSample(Half(406),sample) && fabs(sample->actors[slot].facing-337.5)<0.001);
    actor->x+=3*TILEGLOBAL;WL_PresentCapture(407);CHECK(WL_PresentSample(Half(407),sample));
    CHECK(sample->actors[slot].x==actor->x);
    RemoveActor(actor);WL_PresentCapture(408);CHECK(WL_PresentSample(Half(408),sample));
    CHECK(!sample->actors[slot].identity);
    const int dx[4]={1,0,-1,0},dy[4]={0,-1,0,1};
    for(unsigned direction=0;direction<4;++direction) {
        int nx=20+dx[direction],ny=20+dy[direction],fx=nx+dx[direction],fy=ny+dy[direction];
        pwallx=20;pwally=20;pwalldir=(int)direction*2;pwallstate=127;pwallpos=63;
        tilemap[20][20]=0xc1;tilemap[nx][ny]=1;tilemap[fx][fy]=0;
        WL_PresentReset();WL_PresentCapture(500);
        pwallx=nx;pwally=ny;pwallstate=128;pwallpos=0;
        tilemap[20][20]=0;tilemap[nx][ny]=0xc1;tilemap[fx][fy]=1;WL_PresentCapture(501);
        CHECK(WL_PresentSample(Half(501),sample));
        CHECK(sample->push_x==20 && sample->push_y==20 && sample->tiles[20][20]==0xc1);
        CHECK(sample->tiles[nx][ny]==1 && sample->tiles[fx][fy]==0);
        CHECK(WL_PresentSample(WL_TickNS(502)+1,sample) && sample->push_x==nx && sample->tiles[fx][fy]==1);
        pwallstate=255;pwallpos=63;WL_PresentCapture(502);
        pwallstate=0;tilemap[nx][ny]=0;WL_PresentCapture(503);
        CHECK(WL_PresentSample(Half(503),sample) && sample->tiles[nx][ny]==0xc1 && abs(sample->push_offset-65024)<=1);
        CHECK(WL_PresentSample(WL_TickNS(504)+1,sample) && sample->tiles[nx][ny]==0);
    }
    GameStop();CHECK(!WL_PresentSample(0,sample));free(sample);
    puts("Immutable poses, angle wrap, actor reuse, doors, pushwall crossings, teleport, pause, load, level and death resets passed.");
}
static void Images(void)
{
    uint8_t *pixels=malloc(640*480);wl_present_snapshot *sample=malloc(sizeof(*sample));CHECK(pixels && sample);
    wl_render_target target;int32_t depth[640];CHECK(WL_TargetInit(&target,pixels,640*480,640,480,640));
    CT_Reset(CT_CLASSIC);CHECK(GameStart(0));CT_Key(SDL_SCANCODE_RIGHT,true);CHECK(GameStep(1) && GameStep(2));
    uint32_t images[3],before=State();uint64_t times[]={WL_TickNS(2),Half(2),WL_TickNS(3)};
    for(unsigned i=0;i<3;++i) { CHECK(WL_PresentSample(times[i],sample));CHECK(WL_RenderSnapshot(&target,depth,640,true,16,9,sample,NULL));images[i]=Hash(pixels,640*480); }
    CHECK(images[0]!=images[1] && images[1]!=images[2] && State()==before);
    objtype saved=player;w3r_byte tiles[64][64];doorobj_t doors[64];memcpy(tiles,tilemap,sizeof(tiles));memcpy(doors,doorobjlist,sizeof(doors));
    unsigned door_count=doornum,statics=statcount,offset=pwallpos;
    player.angle=999;player.next=NULL;memset(tilemap,1,sizeof(tilemap));doornum=statcount=0;pwallpos=63;
    CHECK(WL_RenderSnapshot(&target,depth,640,true,16,9,sample,NULL));CHECK(Hash(pixels,640*480)==images[2]);
    player=saved;memcpy(tilemap,tiles,sizeof(tiles));memcpy(doorobjlist,doors,sizeof(doors));doornum=door_count;statcount=statics;pwallpos=offset;CHECK(State()==before);
    sample->camera.angle=NAN;CHECK(!WL_RenderSnapshot(&target,depth,640,true,16,9,sample,NULL));
    CHECK(GameStart(0));player.x=2*TILEGLOBAL+32768;player.y=3*TILEGLOBAL+32768;player.angle=0;player.next=NULL;statcount=0;
    memset(tilemap,0,sizeof(tilemap));for(unsigned i=0;i<64;++i) tilemap[0][i]=tilemap[63][i]=tilemap[i][0]=tilemap[i][63]=1;
    for(unsigned y=1;y<63;++y) tilemap[4][y]=1;
    tilemap[4][3]=128;doornum=1;doorobjlist[0]=(doorobj_t){4,3,0,true,dr_opening,0,0};
    WL_PresentReset();WL_PresentCapture(200);doorobjlist[0].position=65535;WL_PresentCapture(201);
    for(unsigned i=0;i<3;++i) { uint64_t t=i==0?WL_TickNS(201):i==1?Half(201):WL_TickNS(202);CHECK(WL_PresentSample(t,sample));CHECK(WL_RenderSnapshot(&target,depth,640,true,16,9,sample,NULL));images[i]=Hash(pixels,640*480); }
    CHECK(images[0]!=images[1] && images[1]!=images[2]);
    tilemap[4][3]=0xc1;pwallstate=1;pwallpos=0;pwallx=4;pwally=3;pwalldir=0;WL_PresentReset();WL_PresentCapture(210);
    pwallpos=32;WL_PresentCapture(211);
    for(unsigned i=0;i<3;++i) { uint64_t t=i==0?WL_TickNS(211):i==1?Half(211):WL_TickNS(212);CHECK(WL_PresentSample(t,sample));CHECK(WL_RenderSnapshot(&target,depth,640,true,16,9,sample,NULL));images[i]=Hash(pixels,640*480); }
    CHECK(images[0]!=images[1] && images[1]!=images[2]);
    free(pixels);free(sample);GameStop();puts("Intermediate camera, door and pushwall images are distinct; snapshot rays ignore the mutable live world.");
}
static void Rates(const char *directory)
{
    static const unsigned rates[]={60,75,90,120,144,165,180,240};
    uint8_t *pixels=malloc(640*480);wl_present_snapshot *sample=malloc(sizeof(*sample));CHECK(pixels && sample);
    wl_render_target target;int32_t depth[640];CHECK(WL_TargetInit(&target,pixels,640*480,640,480,640));
    for(unsigned r=0;r<sizeof(rates)/sizeof(rates[0]);++r) {
        CT_Reset(CT_CLASSIC);CHECK(NewViewSize(15) && GameStart(0));
        uint64_t tick=0,now=0;
        for(unsigned frame=0;frame<rates[r]*2;++frame) {
            now=WL_FrameDeadline(now,rates[r]);
            while(WL_TickNext(&tick,now)) {
                CT_Key(SDL_SCANCODE_UP,tick<70);CT_Key(SDL_SCANCODE_RIGHT,tick%28<14);
                CT_Key(SDL_SCANCODE_LCTRL,tick%20<5);CHECK(GameStep(tick));
            }
            CHECK(WL_PresentSample(now,sample));uint32_t state=State(),immutable=Hash(WL_PresentCurrent(),sizeof(*sample));
            CHECK(WL_RenderSnapshot(&target,depth,640,true,16,9,sample,NULL));
            CHECK(State()==state && immutable==Hash(WL_PresentCurrent(),sizeof(*sample)));
        }
        CHECK(tick==140 && gamestate.TimeCount==140);
        CHECK(Save_SetDirectory(directory) && Save_Write(r%5,"same 140 ticks",true));
        printf("%u FPS: %u presentation frames, 140 authoritative ticks.\n",rates[r],rates[r]*2);
        GameStop();
    }
    /* Teal fills the HUD gutters, while the central artwork stays exact. */
    CHECK(GameStart(0) && GameDrawPaused());
    CHECK(WL_RenderFrameAspect(&target,depth,640,true,16,9,NULL));
    CHECK(pixels[419*640]==127 && pixels[419*640+639]==127);
    free(pixels);free(sample);GameStop();
}
int main(int argc,char **argv)
{
    CHECK(argc==3);MM_Startup();CHECK(CA_Startup(argv[1]));CHECK(Save_SetDirectory(argv[2]));
    Snapshots();Images();Rates(argv[2]);CA_Shutdown();MM_Shutdown();CHECK(!MM_TotalBytes());return 0;
}
