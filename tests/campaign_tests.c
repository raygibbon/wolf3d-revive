/* SPDX-License-Identifier: GPL-2.0-only */
#include "engine/WL_DEF.h"
#include "engine/ID_CA.h"
#include "engine/ID_IN.h"
#include "engine/ID_PM.h"
#include "engine/ID_VL.h"
#include "engine/WL_MAIN.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(x) do { if (!(x)) { fprintf(stderr,"%d: %s (%s)\n",__LINE__,#x,CA_Error());exit(1); } } while (0)
static void Key(unsigned scan) { IN_KeyEvent(scan,true);IN_KeyEvent(scan,false); }
static void Place(int x,int y,int angle)
{
    player.x=x*TILEGLOBAL+32768;player.y=y*TILEGLOBAL+32768;
    player.tilex=x;player.tiley=y;player.angle=angle;
}
static void TestDifficulty(void)
{
    CHECK(!NewGame(4,0) && !NewGame(2,6));
    for (unsigned ep=0;ep<6;++ep) for (unsigned diff=0;diff<4;++diff) {
        CHECK(GameStartSelected(0,ep,diff));
        unsigned count=diff<2 ? 2 : diff==2 ? 4 : 6;
        CHECK(CA_Map()->number==ep*10 && objcount==count && gamestate.killtotal==(int)count);
        CHECK(gamestate.episode==(int)ep && gamestate.difficulty==(int)diff);
        for (unsigned i=0;i<objcount;++i) if (objlist[i].obclass==mutantobj)
            CHECK(objlist[i].hitpoints==(diff==0 ? 45 : diff==3 ? 65 : 55));
        TakeDamage(20,NULL);CHECK(gamestate.health==(diff==0 ? 95 : 80));
        GameStop();
    }
}
static void TestStats(void)
{
    CHECK(GameStart(0));
    gamestate.killtotal=3;gamestate.killcount=2;
    gamestate.secrettotal=1;gamestate.secretcount=1;
    gamestate.treasuretotal=3;gamestate.treasurecount=3;
    gamestate.TimeCount=70*60+69;
    levelratio_t r=LevelStats();
    CHECK(r.seconds==60 && r.par==90 && r.kill==66 && r.secret==100 && r.treasure==100 && r.bonus==35000);
    gamestate.TimeCount=70*90;r=LevelStats();CHECK(r.bonus==20000);
    gamestate.killtotal=gamestate.secrettotal=gamestate.treasuretotal=0;
    r=LevelStats();CHECK(!r.kill && !r.secret && !r.treasure && !r.bonus);
    gamestate.TimeCount=UINT32_MAX;r=LevelStats();CHECK(r.seconds==5940 && !r.bonus);
    for (unsigned ep=0;ep<6;++ep) {
        static const unsigned first[]={90,90,90,120,150,390};
        gamestate.episode=(int)ep;gamestate.mapon=0;r=LevelStats();CHECK(r.par==first[ep]);
        gamestate.mapon=9;r=LevelStats();CHECK(r.bonus==15000);
        CHECK(NextMap(ep,0,ex_completed)==ep*10+1);
        CHECK(NextMap(ep,0,ex_secretlevel)==ep*10+9);
        static const unsigned back[]={1,1,7,3,5,3};
        CHECK(NextMap(ep,9,ex_completed)==ep*10+back[ep]);
    }
    CHECK(NextMap(6,0,ex_completed)==60);
    GameStop();
}
static void TestProgression(void)
{
    CHECK(GameStart(0));Place(3,2,90);Cmd_Use();CHECK(playstate==ex_playing && tilemap[4][2]==21);
    Place(3,2,0);gamestate.health=37;gamestate.ammo=42;gamestate.keys=3;
    GiveWeapon(wp_chaingun);gamestate.score=1234;
    doorobjlist[0].action=dr_open;doorobjlist[0].position=65535;
    Key(sc_Space);CHECK(GameStep(1));
    CHECK(GameIntermission() && playstate==ex_completed && tilemap[4][2]==22 && gamestate.keys==0);
    CHECK(LevelRatios[0].seconds==0 && LevelRatios[0].bonus==45000 && gamestate.score==46234);
    unsigned t=1;int score=gamestate.score,lives=gamestate.lives;
    CHECK(GameStep(++t) && GameIntermission() && gamestate.score==score);
    Key(sc_Enter);CHECK(GameStep(++t));
    CHECK(!GameIntermission() && CA_Map()->number==1 && gamestate.mapon==1);
    CHECK(gamestate.health==37 && gamestate.ammo==48 && gamestate.weapon==wp_chaingun && gamestate.bestweapon==wp_chaingun);
    CHECK(gamestate.oldscore==score && !gamestate.TimeCount && !gamestate.keys);
    CHECK(doorobjlist[0].action==dr_closed && !doorobjlist[0].position);
    /* Die on a later floor: return to that floor, restore its entry score. */
    GivePoints(100);TakeDamage(100,NULL);
    while (gamestate.died && t<500) CHECK(GameStep(++t));
    CHECK(!gamestate.died && CA_Map()->number==1 && gamestate.score==score && gamestate.lives==lives-1);
    CHECK(gamestate.health==100 && gamestate.ammo==8 && gamestate.weapon==wp_pistol && !gamestate.keys);
    /* Secret elevator raw area marker, then the original episode return. */
    Place(3,2,0);((maptype *)CA_Map())->planes[0][2*64+3]=107;
    Key(sc_Space);CHECK(GameStep(++t) && GameIntermission() && playstate==ex_secretlevel);
    Key(sc_Enter);CHECK(GameStep(++t) && CA_Map()->number==9);
    Place(3,2,0);Key(sc_Space);CHECK(GameStep(++t) && GameIntermission());
    CHECK(gamestate.score==score+60000+15000);
    Key(sc_Enter);CHECK(GameStep(++t) && CA_Map()->number==1);
    GameStop();
}
static void TestPushwalls(void)
{
    static const int dx[]={1,0,-1,0},dy[]={0,-1,0,1};
    for (unsigned d=0;d<4;++d) {
        CHECK(GameStart(0));Place(30-dx[d]*2,30-dy[d]*2,(int)d*90);
        tilemap[30][30]=1;pushable[30][30]=true;
        int nx=30+dx[d],ny=30+dy[d];
        tilemap[nx][ny]=1;PushWall(30,30,(int)d*2);CHECK(!pwallstate && pushable[30][30]);
        tilemap[nx][ny]=0;staticblock[nx][ny]=true;PushWall(30,30,(int)d*2);CHECK(!pwallstate);
        staticblock[nx][ny]=false;actorat[nx][ny]=&objlist[0];PushWall(30,30,(int)d*2);CHECK(!pwallstate);actorat[nx][ny]=NULL;
        PushWall(30,30,(int)d*2);CHECK(pwallstate==1 && !pwallpos && gamestate.secretcount==1 && !pushable[30][30]);
        CHECK(tilemap[30][30]==193 && tilemap[nx][ny]==1);
        objtype mover=player;mover.dir=(int)d*2;mover.tilex=30-dx[d];mover.tiley=30-dy[d];CHECK(!TryWalk(&mover));
        rayhit first,middle;CHECK(AsmRefresh((int)d*900,&first));
        for (unsigned i=0;i<63;++i) MovePWalls();
        CHECK(pwallstate==64 && pwallpos==32 && AsmRefresh((int)d*900,&middle));
        CHECK(first.side==(d%2 ? 3 : 4) && middle.side==first.side && middle.height<first.height);
        CHECK(d%2 ? abs(middle.y-first.y)==32768 : abs(middle.x-first.x)==32768);
        for (unsigned i=0;i<64;++i) MovePWalls();
        CHECK(pwallstate==128 && !tilemap[30][30] && areaMap[30][30]==areaMap[player.tilex][player.tiley]);
        CHECK(tilemap[nx][ny]==193 && tilemap[nx+dx[d]][ny+dy[d]]==1);
        for (unsigned i=0;i<128;++i) MovePWalls();
        CHECK(!pwallstate && !tilemap[nx][ny] && tilemap[nx+dx[d]][ny+dy[d]]==1);
        CHECK(ThreeDRefresh());
        GameStop();CHECK(!pwallstate && !pushable[30][30]);
    }
    CHECK(GameStart(0));tilemap[30][30]=1;tilemap[32][30]=1;pushable[30][30]=true;
    PushWall(30,30,east);for (unsigned i=0;i<127;++i) MovePWalls();
    CHECK(!pwallstate && !tilemap[30][30] && tilemap[31][30]==1); /* One-tile stop. */
    GameStop();
}
static void TestOwned(void)
{
    unsigned supported=0,unsupported=0;
    for (unsigned diff=0;diff<4;++diff) for (unsigned ep=0;ep<6;++ep) for (unsigned map=0;map<10;++map) {
        CHECK(NewGame(diff,ep));gamestate.mapon=(int)map;
        if (!SetupGameLevel()) {
            CHECK(strstr(CA_Error(),"Unported boss/ghost/victory")!=NULL);
            ++unsupported;
        } else { CHECK(CA_Map()->number==ep*10+map);CHECK(ThreeDRefresh() && DrawStatus());++supported; }
        ShutdownGameLevel();
    }
    printf("Owned WL6: %u normal maps validated; %u maps explicitly require unported encounters.\n",supported,unsupported);
    CHECK(supported==51*4 && unsupported==9*4);
}
int main(int argc,char **argv)
{
    CHECK(argc==2 || argc==3);MM_Startup();CHECK(CA_Startup(argv[1]));CHECK(VL_SetPalette(gamepal,768));
    if (argc==3) TestOwned();
    else { TestDifficulty();TestStats();TestProgression();TestPushwalls(); }
    GameStop();CA_Shutdown();MM_Shutdown();CHECK(!MM_TotalBytes());
    puts("Campaign selection, difficulty, exits, ratios, inventory, restarts and pushwalls passed.");return 0;
}
