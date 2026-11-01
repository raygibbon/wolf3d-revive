/* SPDX-License-Identifier: GPL-2.0-only */
#include "engine/WL_DEF.h"
#include "engine/ID_CA.h"
#include "engine/ID_US_1.h"
#include "engine/ID_VL.h"
#include <stdio.h>
#include <stdlib.h>
#define CHECK(x) do { if (!(x)) { fprintf(stderr,"%d: %s (%s)\n",__LINE__,#x,CA_Error());exit(1); } } while (0)
extern statetype s_bossstand,s_bosschase1,s_bossshoot1,s_bossdie1,s_bossdie4;
extern statetype s_gretelstand,s_gretelchase1,s_gretelshoot1,s_greteldie1,s_greteldie4;
static unsigned sounds,shots;
static soundnames lastsound;
static const objtype *lastsource;
static void Sound(soundnames sound,const objtype *source)
{
    ++sounds;lastsound=sound;lastsource=source;
    if (sound==BOSSFIRESND || sound==NAZIFIRESND) ++shots;
}
static void PlacePlayer(int x,int y)
{
    player.x=x*TILEGLOBAL+32768;player.y=y*TILEGLOBAL+32768;
    player.tilex=x;player.tiley=y;player.angle=0;ConnectAreas();
}
static void Start(unsigned diff)
{
    CHECK(NewGame(diff,0));US_InitRndT(0);CHECK(SetupGameLevel());
    CHECK(objcount==6 && gamestate.killtotal==6);
    sounds=shots=0;
}
static void TestSpawns(void)
{
    static const int hp[]={850,950,1050,1200};
    static const int sprites[]={SPR_BLINKY_W1,SPR_CLYDE_W1,SPR_PINKY_W1,SPR_INKY_W1};
    for (unsigned diff=0;diff<4;++diff) {
        Start(diff);
        CHECK(objlist[0].state==&s_bossstand && objlist[0].obclass==bossobj);
        CHECK(objlist[1].state==&s_gretelstand && objlist[1].obclass==gretelobj);
        for (unsigned i=0;i<2;++i) {
            CHECK(objlist[i].hitpoints==hp[diff] && objlist[i].speed==SPDPATROL);
            CHECK(objlist[i].flags==(FL_SHOOTABLE|FL_AMBUSH));
            CHECK(objlist[i].dir==(i ? north : south) && !objlist[i].ticcount);
        }
        for (unsigned i=0;i<4;++i) {
            objtype *ghost=&objlist[i+2];
            CHECK(ghost->obclass==ghostobj && ghost->state->shapenum==sprites[i]);
            CHECK(ghost->dir==east && ghost->speed==SPDDOG && ghost->flags==FL_AMBUSH);
            CHECK(!ghost->hitpoints && actorat[ghost->tilex][ghost->tiley]==ghost);
            DamageActor(ghost,10000);CHECK(!gamestate.killcount && ghost->flags==FL_AMBUSH);
        }
        /* Stand states draw no RNG; four ghost states draw once each. */
        CHECK(US_RndT()==241);
        ShutdownGameLevel();
    }
}
static void TestBoss(unsigned which)
{
    Start(2);objtype *ob=&objlist[which];
    statetype *chase=which ? &s_gretelchase1 : &s_bosschase1;
    statetype *shoot=which ? &s_gretelshoot1 : &s_bossshoot1;
    statetype *die=which ? &s_greteldie1 : &s_bossdie1;
    statetype *dead=which ? &s_greteldie4 : &s_bossdie4;
    /* Ambush wakeup requires sight; hearing alone cannot wake a boss. */
    madenoise=true;tilemap[ob->tilex][ob->tiley+1]=1;
    PlacePlayer(ob->tilex,ob->tiley+2);
    CHECK(!SightPlayer(ob) && !ob->temp2 && (ob->flags&FL_AMBUSH));
    tilemap[ob->tilex][ob->tiley+1]=0;
    PlacePlayer(ob->tilex,ob->tiley+(which ? -2 : 2));
    US_InitRndT(0);CHECK(!SightPlayer(ob) && ob->temp2==1);
    CHECK(SightPlayer(ob) && ob->state==chase && ob->speed==1536);
    CHECK(lastsound==(which ? KEINSND : GUTENTAGSND) && !lastsource);
    CHECK(!(ob->flags&FL_AMBUSH) && (ob->flags&FL_ATTACKMODE));
    CHECK(US_RndT()==8); /* No sight-sound RNG. */
    /* Close-range original chase probability always starts the burst. */
    PlacePlayer(ob->tilex+1,ob->tiley);ob->distance=0;ob->dir=nodir;
    T_Chase(ob);CHECK(ob->state==shoot && ob->ticcount==30);
    ob->speed=0;shots=0;
    for (unsigned tic=1;tic<=99;++tic) {
        gamestate.health=100;gamestate.died=false;
        DoActor(ob);
        CHECK(shots==(tic<40 ? 0U : tic>=90 ? 6U : (tic-30)/10));
        CHECK(ob->state!=chase);
    }
    gamestate.victoryflag=true;DoActor(ob);CHECK(ob->state==chase && shots==6);
    gamestate.victoryflag=false;
    /* Distant Hans shots use dist*2/3; Gretel retains normal accuracy. */
    PlacePlayer(ob->tilex+6,ob->tiley);ob->flags|=FL_VISABLE;
    gamestate.health=100;US_InitRndT(0);T_Shoot(ob);
    CHECK(gamestate.health==94 && lastsource==ob);
    gamestate.health=100;US_InitRndT(23);T_Shoot(ob);
    CHECK(gamestate.health==(which ? 100 : 95));
    CHECK(US_RndT()==(which ? 89 : 77));
    /* Kill and key drop happen immediately; corpse animation lasts 45 tics. */
    int score=gamestate.score;DamageActor(ob,10000);
    CHECK(ob->state==die && gamestate.score==score+5000 && gamestate.killcount==1);
    CHECK(!(ob->flags&FL_SHOOTABLE) && (ob->flags&FL_NONMARK));
    CHECK(!actorat[ob->tilex][ob->tiley]);
    CHECK(statcount==1 && statobjlist[0].itemnumber==bo_key1);
    CHECK(statobjlist[0].tilex==ob->tilex && statobjlist[0].tiley==ob->tiley);
    unsigned before=sounds;
    for (unsigned tic=0;tic<45;++tic) DoActor(ob);
    CHECK(ob->state==dead && !ob->ticcount && sounds==before+1);
    CHECK(lastsound==(which ? MEINSND : MUTTISND) && !lastsource);
    CHECK(!gamestate.victoryflag && playstate==ex_playing);
    for (unsigned tic=0;tic<100;++tic) DoActor(ob);
    CHECK(ob->state==dead && gamestate.killcount==1 && statcount==1);
    PlacePlayer(ob->tilex,ob->tiley);GetBonus(&statobjlist[0]);CHECK(gamestate.keys==1);
    ShutdownGameLevel();
}
static void TestGhosts(void)
{
    Start(2);
    for (unsigned i=2;i<6;++i) {
        objtype *ob=&objlist[i];PlacePlayer(2,2);
        statetype *first=ob->state;ob->speed=0;ob->ticcount=10;
        for (unsigned tic=0;tic<9;++tic) DoActor(ob);
        CHECK(ob->state==first);
        DoActor(ob);CHECK(ob->state==first->next && ob->ticcount==10);
        for (unsigned tic=0;tic<10;++tic) DoActor(ob);
        CHECK(ob->state==first && ob->ticcount==10);
        ob->speed=SPDDOG;
        PlacePlayer(ob->tilex+1,ob->tiley);
        int32_t x=ob->x;int health=gamestate.health;
        ob->dir=east;ob->distance=TILEGLOBAL;MoveObj(ob,SPDDOG);
        CHECK(gamestate.health==health-2 && ob->x==x && ob->distance==TILEGLOBAL);
        PlacePlayer(2,2);MoveObj(ob,SPDDOG);
        CHECK(ob->x==x+SPDDOG && ob->distance==TILEGLOBAL-SPDDOG);
        /* Walls and shootable reservations block ghosts. Other ghosts do not. */
        ob->dir=east;tilemap[ob->tilex+1][ob->tiley]=1;CHECK(!TryWalk(ob));
        tilemap[ob->tilex+1][ob->tiley]=0;
        actorat[ob->tilex+1][ob->tiley]=&objlist[0];CHECK(!TryWalk(ob));
        actorat[ob->tilex+1][ob->tiley]=&objlist[2];CHECK(TryWalk(ob));
    }
    /* Closed-door wait must not turn the negative distance into a huge move. */
    objtype *ob=&objlist[2];ob->distance=-1;ob->dir=east;
    int32_t x=ob->x;T_Ghosts(ob);
    CHECK(ob->x==x && ob->distance==-1 && doorobjlist[0].action==dr_opening);
    doorobjlist[0].action=dr_open;T_Ghosts(ob);
    CHECK(ob->x==x+SPDDOG && ob->distance==TILEGLOBAL-SPDDOG);
    ShutdownGameLevel();
}
static void TestDeathSoundRNG(void)
{
    objtype ob={.obclass=guardobj};
    gamestate.episode=0;gamestate.mapon=0;US_InitRndT(0);A_DeathScream(&ob);
    CHECK(lastsound==DEATHSCREAM1SND && US_RndT()==109);
    gamestate.mapon=9;US_InitRndT(0);A_DeathScream(&ob);
    CHECK(lastsound==DEATHSCREAM7SND && US_RndT()==220);
    /* The rare draw returns before normal sound selection. */
    US_InitRndT(255);A_DeathScream(&ob);CHECK(lastsound==DEATHSCREAM6SND && US_RndT()==8);
    ob.obclass=bossobj;US_InitRndT(255);A_DeathScream(&ob);
    CHECK(lastsound==MUTTISND && US_RndT()==8);
    /* It is absolute map index nine, not every episode's secret floor. */
    gamestate.episode=1;US_InitRndT(0);A_DeathScream(&ob);CHECK(US_RndT()==8);
    ob.obclass=officerobj;gamestate.episode=0;
    US_InitRndT(0);A_DeathScream(&ob);CHECK(lastsound==NEINSOVASSND && US_RndT()==109);
    /* A disconnected sound sink must still consume the same draws. */
    SetGameSoundHook(NULL);ob.obclass=guardobj;
    US_InitRndT(0);A_DeathScream(&ob);CHECK(US_RndT()==220);
    SetGameSoundHook(Sound);
}
int main(int argc,char **argv)
{
    CHECK(argc==2);MM_Startup();CHECK(CA_Startup(argv[1]));CHECK(VL_SetPalette(gamepal,768));
    SetGameSoundHook(Sound);
    TestSpawns();TestBoss(0);TestBoss(1);TestGhosts();TestDeathSoundRNG();
    SetGameSoundHook(NULL);GameStop();CA_Shutdown();MM_Shutdown();CHECK(!MM_TotalBytes());
    puts("Hans, Gretel, ghosts, sound events and original death RNG passed.");return 0;
}
