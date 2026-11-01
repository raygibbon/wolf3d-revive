/* SPDX-License-Identifier: GPL-2.0-only */
#include "engine/WL_DEF.h"
#include "engine/ID_CA.h"
#include "engine/ID_IN.h"
#include "engine/ID_US_1.h"
#include "engine/ID_VL.h"
#include "engine/ID_PM.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(x) do { if (!(x)) { fprintf(stderr,"%d: %s (%s)\n",__LINE__,#x,CA_Error());exit(1); } } while (0)
static uint64_t ticks;
static soundnames events[1024];static unsigned eventcount;
static void Sound(soundnames sound,const objtype *ob)
{
    (void)ob;CHECK(eventcount<1024);events[eventcount++]=sound;
}
static unsigned Live(void)
{
    unsigned count=0;for (objtype *ob=player.next;ob;ob=ob->next) { CHECK(++count<=MAXACTORS);CHECK(ob->state); }
    return count;
}
static void Place(int x,int y)
{
    player.tilex=x;player.tiley=y;player.x=x*TILEGLOBAL+32768;
    player.y=y*TILEGLOBAL+32768;player.angle=90;ConnectAreas();
}
static void Start(unsigned ep,unsigned diff)
{
    ticks=0;CHECK(GameStartSelected(0,ep,diff));
    ShutdownGameLevel();gamestate.mapon=8;CHECK(SetupGameLevel());eventcount=0;
    CHECK(objcount==5 && Live()==5 && gamestate.killtotal==5);
}
static void Tick(void) { CHECK(GameStep(++ticks)); }
static void Tap(unsigned key) { IN_KeyEvent(key,true);IN_KeyEvent(key,false);Tick(); }
static void TestSpawns(void)
{
    const int hp[4][5]={{850,200,800,850,850},{950,300,950,950,950},
        {1550,400,1050,1050,1050},{2400,500,1200,1200,1200}};
    const int classes[]={schabbobj,fakeobj,mechahitlerobj,giftobj,fatobj};
    statetype *const states[]={&s_schabbstand,&s_fakestand,&s_mechastand,&s_giftstand,&s_fatstand};
    const int dirs[]={south,north,south,north,south};
    for (unsigned diff=0;diff<4;++diff) {
        Start(0,diff);
        for (unsigned i=0;i<5;++i) {
            objtype *ob=&objlist[i];CHECK(ob->obclass==classes[i] && ob->state==states[i]);
            CHECK(ob->hitpoints==hp[diff][i] && ob->dir==dirs[i] && ob->speed==512);
            CHECK(ob->flags==(FL_SHOOTABLE|FL_AMBUSH));
            FirstSighting(ob);CHECK(ob->speed==1536 && (ob->flags&FL_ATTACKMODE));
        }
        CHECK(s_schabbdie2.tictime==5 && s_hitlerdie2.tictime==5);
        GameStop();
    }
}
static objtype *Projectile(int kind,statetype *state)
{
    objtype *ob=GetNewActor();CHECK(ob);ob->state=state;ob->ticcount=state->tictime;
    ob->obclass=kind;ob->flags=FL_NEVERMARK;ob->active=true;
    ob->x=20*TILEGLOBAL+32768;ob->y=20*TILEGLOBAL+32768;
    ob->tilex=ob->tiley=20;ob->dir=nodir;ob->angle=0;return ob;
}
static void TestProjectileRules(void)
{
    Start(0,2);objtype *boss=&objlist[0];Place(14,30);
    US_InitRndT(0);T_SchabbThrow(boss);objtype *p=boss;
    for (objtype *ob=player.next;ob;ob=ob->next) p=ob;
    CHECK(p->obclass==needleobj && p->speed==0x2000 && p->ticcount==1);
    CHECK(p->x==boss->x && p->y==boss->y && p->angle==0 && p->flags==FL_NONMARK);
    CHECK(events[0]==SCHABBSTHROWSND && US_RndT()==8);
    int32_t x=p->x;DoActor(p);CHECK(p->x==x+FixedByFrac(8192,sintable[90]));RemoveActor(p);
    /* Player-hit damage, original strict square, and wall-before-player order. */
    const int classes[]={needleobj,rocketobj,fireobj};
    statetype *states[]={&s_needle1,&s_rocket,&s_fire1};
    const int damage[]={21,31,1};
    for (unsigned i=0;i<3;++i) {
        p=Projectile(classes[i],states[i]);Place(20,20);gamestate.health=100;US_InitRndT(0);
        T_Projectile(p);CHECK(!p->state && gamestate.health==100-damage[i]);
        RemoveActor(p);CHECK(Live()==5);
        p=Projectile(classes[i],states[i]);player.x=p->x+0xc000;US_InitRndT(0);
        T_Projectile(p);CHECK(p->state && US_RndT()==8);RemoveActor(p);
        p=Projectile(classes[i],states[i]);p->x=21*TILEGLOBAL-1;p->speed=8192;
        tilemap[21][20]=1;Place(21,20);gamestate.health=100;US_InitRndT(0);
        T_Projectile(p);CHECK(gamestate.health==100 && US_RndT()==8);
        if (classes[i]==rocketobj) {
            CHECK(p->state==&s_boom1 && events[eventcount-1]==MISSILEHITSND);
            for (unsigned tic=0;tic<15;++tic) DoActor(p);
            CHECK(!p->state && Live()==5);
        } else { CHECK(!p->state);RemoveActor(p); }
        tilemap[21][20]=0;
    }
    /* Flames move on state actions, not on each simulation tic. */
    Place(2,2);T_FakeFire(&objlist[1]);
    for (objtype *ob=player.next;ob;ob=ob->next) p=ob;
    CHECK(p->speed==0x1200 && p->flags==FL_NEVERMARK && events[eventcount-1]==FLAMETHROWERSND);
    p->angle=0;p->x=20*TILEGLOBAL+32768;p->y=20*TILEGLOBAL+32768;p->tilex=p->tiley=20;
    x=p->x;for (unsigned tic=0;tic<13;++tic) DoActor(p);
    CHECK(p->x==x+3*FixedByFrac(0x1200,sintable[90]));RemoveActor(p);
    /* Closed doors and static blocks stop projectiles; live actors do not. */
    p=Projectile(needleobj,&s_needle1);tilemap[20][20]=128;CHECK(!ProjectileTryMove(p));
    doorobjlist[0].action=dr_open;CHECK(ProjectileTryMove(p));tilemap[20][20]=0;
    staticblock[20][20]=true;CHECK(!ProjectileTryMove(p));staticblock[20][20]=false;
    actorat[20][20]=boss;CHECK(ProjectileTryMove(p));actorat[20][20]=NULL;
    p->x=1;p->angle=180;p->speed=8192;T_Projectile(p);CHECK(!p->state);RemoveActor(p);
    GameStop();
}
static void TestBurstsAndMorph(void)
{
    Start(2,2);Place(18,27);objtype *fake=&objlist[1];NewState(fake,&s_fakeshoot1);
    unsigned before=Live();
    for (unsigned tic=0;tic<64;++tic) DoActor(fake);
    CHECK(Live()==before+8 && fake->state==&s_fakeshoot9);
    CHECK(eventcount==8);for (unsigned i=0;i<8;++i) CHECK(events[i]==FLAMETHROWERSND);
    Start(5,2);Place(42,32);objtype *fat=&objlist[4];NewState(fat,&s_fatshoot1);
    for (unsigned tic=0;tic<79;++tic) { gamestate.health=100;DoActor(fat); }
    CHECK(eventcount==4 && events[0]==MISSILEFIRESND && Live()==6);
    gamestate.health=100;DoActor(fat);CHECK(eventcount>=5 && events[4]==MISSILEFIRESND);
    static const int realhp[]={500,700,800,900};
    for (unsigned diff=0;diff<4;++diff) {
        Start(2,diff);objtype *mecha=&objlist[2];
        mecha->x+=123;mecha->y+=456;mecha->distance=12345;mecha->dir=northeast;
        DamageActor(mecha,10000);CHECK(mecha->state==&s_mechadie1 && gamestate.score==5000);
        unsigned flags=mecha->flags;US_InitRndT(0);
        for (unsigned tic=0;tic<30;++tic) DoActor(mecha);
        CHECK(mecha->state==&s_mechadie4 && Live()==6 && gamestate.killtotal==5);
        objtype *hitler=mecha;for (objtype *ob=player.next;ob;ob=ob->next) hitler=ob;
        CHECK(hitler!=mecha && hitler->obclass==realhitlerobj && hitler->state==&s_hitlerchase1);
        CHECK(hitler->x==mecha->x && hitler->y==mecha->y && hitler->distance==12345);
        CHECK(hitler->dir==northeast && hitler->flags==(flags|FL_SHOOTABLE));
        CHECK(hitler->speed==2560 && hitler->hitpoints==realhp[diff] && hitler->ticcount==2);
        CHECK(US_RndT()==109);DamageActor(hitler,10000);
        CHECK(gamestate.score==10000 && gamestate.killcount==2 && hitler->state==&s_hitlerdie1);
    }
    GameStop();
}
static void TestPoolAndKiller(void)
{
    Start(0,2);objtype *slot=NULL;
    for (unsigned i=0;i<1000;++i) {
        A_Smoke(&objlist[0]);objtype *smoke=player.next;while (smoke->next) smoke=smoke->next;
        if (slot) CHECK(slot==smoke);
        slot=smoke;
        for (unsigned tic=0;tic<15;++tic) DoActor(smoke);
        CHECK(Live()==5 && !smoke->state);
    }
    objtype *p=Projectile(needleobj,&s_needle1);Place(20,20);gamestate.health=1;US_InitRndT(0);
    T_Projectile(p);CHECK(gamestate.died && killerobj && killerobj->x==p->x);
    int32_t x=killerobj->x;RemoveActor(p);objtype *reused=GetNewActor();CHECK(reused==p);
    reused->state=&s_smoke1;reused->x=1;CHECK(killerobj->x==x);RemoveActor(reused);
    while (Live()<MAXACTORS) {
        objtype *ob=GetNewActor();CHECK(ob);ob->state=&s_smoke1;ob->flags=FL_NEVERMARK;
    }
    unsigned count=Live(),sounds=eventcount;US_InitRndT(0);T_GiftThrow(&objlist[3]);
    CHECK(ActorAllocationFailed() && Live()==count && eventcount==sounds && US_RndT()==8);
    CHECK(strstr(CA_Error(),"pool exhausted")!=NULL);
    GameStop();CHECK(!player.next && !objcount && !ActorAllocationFailed());
}
static void TestCamera(unsigned index,unsigned episode)
{
    Start(episode,2);objtype *boss=&objlist[index];Place(boss->tilex,boss->tiley+2);
    if (boss->obclass==mechahitlerobj) {
        DamageActor(boss,10000);for (unsigned i=0;i<30;++i) DoActor(boss);
        for (objtype *ob=player.next;ob;ob=ob->next) if (ob->obclass==realhitlerobj) boss=ob;
    }
    int32_t killx=player.x,killy=player.y;StartDamageFlash(1000);DamageActor(boss,10000);
    bool sawcamera=false,sawreplay=false;
    while (!GameVictoryPresentation() && ticks<2000) {
        gamestate.health=100;Tick();
        if (DeathCameraActive()) {
            sawcamera=true;
            if (player.state!=&s_deathcam) CHECK(!memcmp(palette,gamepal,768));
        }
        if (gamestate.victoryflag && !DeathCameraActive()) sawreplay=true;
    }
    CHECK(sawcamera && sawreplay && GameVictoryPresentation() && playstate==ex_victorious);
    CHECK(gamestate.killx==killx && gamestate.killy==killy);
    CHECK(player.x!=killx || player.y!=killy);CHECK(!GameFinished());
    CHECK(player.state==&s_deathcam);
    unsigned map=CA_Map()->number;Tap(sc_Enter);CHECK(!GameFinished());
    Tap(sc_RightArrow);Tap(sc_LeftArrow);Tap(sc_Escape);CHECK(GameFinished() && CA_Map()->number==map);
    GameStop();
}
static void TestBJ(unsigned episode)
{
    Start(episode,2);Place(50,21);Thrust(90,TILEGLOBAL);
    CHECK(gamestate.victoryflag && Live()==6);
    objtype *bj=player.next;while (bj->next) bj=bj->next;
    CHECK(bj->obclass==bjobj && bj->temp1==6 && bj->dir==north);
    bool jump=false;
    while (!GameVictoryPresentation() && ticks<1500) {
        Tick();if (bj->state->shapenum==SPR_BJ_JUMP4) jump=true;
    }
    CHECK(jump && playstate==ex_victorious && GameVictoryPresentation() && player.angle==270);
    unsigned yells=0;for (unsigned i=0;i<eventcount;++i) if (events[i]==YEAHSND) ++yells;
    CHECK(yells==1);
    Tap(sc_Enter);Tap(sc_RightArrow);Tap(sc_Escape);CHECK(GameFinished());GameStop();
}
static void TestReusedSpriteOrder(void)
{
    Start(0,2);
    for (objtype *ob=player.next;ob;) { objtype *next=ob->next;RemoveActor(ob);ob=next; }
    statetype first={false,SPR_BOSS_W1,0,NULL,NULL,NULL};
    statetype second={false,SPR_GRETEL_W1,0,NULL,NULL,NULL};
    objtype *old=GetNewActor();CHECK(old);old->state=&first;
    objtype *earlier=GetNewActor();CHECK(earlier);earlier->state=&second;
    RemoveActor(old);objtype *later=GetNewActor();CHECK(later==old);later->state=&first;
    CHECK(player.next==earlier && earlier->next==later && !later->next);
    earlier->x=later->x=4*TILEGLOBAL+32768;earlier->y=later->y=2*TILEGLOBAL+32768;
    earlier->tilex=later->tilex=4;earlier->tiley=later->tiley=2;
    w3r_byte *a=(w3r_byte *)PM_GetPage(PMSpriteStart+SPR_BOSS_W1,NULL);
    w3r_byte *b=(w3r_byte *)PM_GetPage(PMSpriteStart+SPR_GRETEL_W1,NULL);
    memset(a+14,10,32);memset(b+14,20,32);Place(2,2);player.angle=0;gamestate.victoryflag=true;
    CHECK(ThreeDRefresh());CHECK(framebuffer[(VIEWY+61)*320+VIEWX+119]==10);
    GameStop();
}
static void TestCameraTiming(void)
{
    Start(1,2);gamestate.killx=player.x;gamestate.killy=player.y;
    A_StartDeathCam(&objlist[0]);unsigned count=0;
    while (DeathCameraActive() && count<1000) { CHECK(StepDeathCamera());++count; }
    /* 100 wait + ceil(131071/(64000/70)) grey fizzle + 300 acknowledgement
       + ceil(131071/(64000/20)) viewport replay fizzle. */
    CHECK(count==585 && player.state==&s_deathcam && player.temp1==0);
    A_StartDeathCam(&objlist[0]);
    for (unsigned i=0;i<99;++i) { CHECK(StepDeathCamera());CHECK(playstate==ex_playing); }
    CHECK(StepDeathCamera() && playstate==ex_victorious);GameStop();
}
static void TestVictoryStats(void)
{
    Start(0,2);gamestate.score=1234;
    for (unsigned i=0;i<8;++i) LevelRatios[i]=(levelratio_t){80+i,20+i,50+i,61+i,0,0};
    CHECK(DrawVictory() && gamestate.score==1234);
    CHECK(framebuffer[112*320+208]==L_NUM0PIC+8 && framebuffer[112*320+224]==L_NUM0PIC+3);
    CHECK(framebuffer[128*320+208]==L_NUM0PIC+2 && framebuffer[128*320+224]==L_NUM0PIC+3);
    CHECK(framebuffer[144*320+208]==L_NUM0PIC+5 && framebuffer[144*320+224]==L_NUM0PIC+3);
    CHECK(framebuffer[64*320+112]==L_NUM0PIC && framebuffer[64*320+128]==L_NUM0PIC+8);
    CHECK(framebuffer[64*320+152]==L_NUM0PIC+3 && framebuffer[64*320+168]==L_NUM0PIC+6);
    for (unsigned i=0;i<8;++i) LevelRatios[i].seconds=5940;
    CHECK(DrawVictory() && framebuffer[64*320+112]==L_NUM0PIC+9 && framebuffer[64*320+168]==L_NUM0PIC+9);
    gamestate.difficulty=0;CHECK(DrawVictory() && framebuffer[64*320+240]==127);
    GameStop();
}
static void TestCinematicControls(void)
{
    Start(1,2);
    for (objtype *ob=player.next;ob;) { objtype *next=ob->next;RemoveActor(ob);ob=next; }
    gamestate.victoryflag=true;player.state=&s_deathcam;
    int32_t x=player.x,y=player.y;int angle=player.angle,ammo=gamestate.ammo;
    IN_KeyEvent(sc_UpArrow,true);IN_KeyEvent(sc_RightArrow,true);
    IN_KeyEvent(sc_Space,true);IN_KeyEvent(0x1d,true);US_InitRndT(0);
    for (unsigned i=0;i<200;++i) Tick();
    CHECK(player.x==x && player.y==y && player.angle==angle && gamestate.ammo==ammo);
    CHECK(US_RndT()==8); /* A death camera has no face-look RNG thinker. */
    GameStop();
}
static void TestRestartAndArticles(void)
{
    Start(1,2);gamestate.oldscore=1234;TakeDamage(100,NULL);
    while (gamestate.died && ticks<600) Tick();
    CHECK(!gamestate.died && gamestate.mapon==8 && gamestate.score==1234 && objlist[0].hitpoints==1550);
    for (unsigned ep=0;ep<6;++ep) {
        gamestate.episode=(int)ep;CHECK(StartEndText());
        bool done=false;IN_KeyEvent(sc_RightArrow,true);IN_KeyEvent(sc_RightArrow,false);
        CHECK(StepEndText(&done) && !done);ResetEndText();
    }
    w3r_byte *article=grsegs[T_ENDART1];size_t size=grsizes[T_ENDART1];
    w3r_byte saved=article[1];article[1]='X';gamestate.episode=0;
    CHECK(!StartEndText());article[1]=saved;CHECK(StartEndText());
    size_t at=0;while (at+1<size && !(article[at]=='^' && article[at+1]=='E')) ++at;
    CHECK(at+1<size);article[at+1]='X';CHECK(!StartEndText());article[at+1]='E';
    ResetEndText();GameStop();
}
int main(int argc,char **argv)
{
    CHECK(argc==2);MM_Startup();CHECK(CA_Startup(argv[1]));CHECK(VL_SetPalette(gamepal,768));
    SetGameSoundHook(Sound);TestSpawns();TestProjectileRules();TestBurstsAndMorph();TestPoolAndKiller();
    TestCamera(0,1);TestCamera(2,2);TestCamera(3,3);TestCamera(4,5);TestBJ(0);TestBJ(4);
    TestReusedSpriteOrder();TestCameraTiming();TestVictoryStats();TestCinematicControls();TestRestartAndArticles();SetGameSoundHook(NULL);GameStop();CA_Shutdown();MM_Shutdown();CHECK(!MM_TotalBytes());
    puts("Remaining bosses, projectiles, morph, allocation, death cameras, BJ, all episode endings and restart passed.");return 0;
}
