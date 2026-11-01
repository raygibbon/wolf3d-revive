/* SPDX-License-Identifier: GPL-2.0-only */
#include "engine/WL_DEF.h"
#include "engine/ID_CA.h"
#include "engine/ID_PM.h"
#include "engine/ID_US_1.h"
#include "engine/ID_IN.h"
#include "engine/ID_VL.h"
#include "engine/WL_MAIN.h"
#include "engine/WL_MENU.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#define CHECK(x) do { if (!(x)) { fprintf(stderr,"%d: %s (%s)\n",__LINE__,#x,CA_Error());exit(1); } } while (0)
extern statetype s_grdstand,s_grdpath1,s_grdchase1,s_grdshoot1,s_grdshoot2,s_grdpain,s_grdpain1,s_grddie1,s_grddie4,s_dogjump1,s_dogjump2;
static objtype *Guard(void) { for (unsigned i=0;i<objcount;++i) if (objlist[i].obclass==guardobj) return &objlist[i];return NULL; }
static void TestRandom(void)
{
    const int expected[]={8,109,220,222,241,149,107,75};
    US_InitRndT(0);for(unsigned i=0;i<8;++i) CHECK(US_RndT()==expected[i]);
    US_InitRndT(255);CHECK(US_RndT()==0);CHECK(US_RndT()==8);
    US_InitRndT(0);int first=US_RndT();for(unsigned i=0;i<255;++i) (void)US_RndT();CHECK(US_RndT()==first);
}
static void TestSpawn(void)
{
    CHECK(GameStart(0));CHECK(objcount==4 && statcount==8 && gamestate.killtotal==3);
    CHECK(gamestate.health==100 && gamestate.ammo==8 && gamestate.lives==3);
    CHECK(GameStart(1));CHECK(objlist[1].ticcount==109%20);
    CHECK(GameStart(256));CHECK(objlist[1].ticcount==8%20);
    CHECK(gamestate.weapon==wp_pistol && gamestate.treasuretotal==3);
    objtype *guard=Guard();CHECK(guard && guard->tilex==5 && guard->tiley==2 && guard->dir==west && guard->hitpoints==25 && guard->state==&s_grdstand);
    CHECK(objlist[1].obclass==dogobj && objlist[1].tilex==8 && objlist[1].tiley==5 && objlist[1].x==9*65536+32768);
    CHECK(!actorat[9][5] && actorat[8][5]==&objlist[1]);
    CHECK(staticblock[3][4] && !staticblock[3][5]);
    objtype test=player;test.x=3*65536+32768;test.y=4*65536+32768;CHECK(!TryMove(&test));
    test.y=5*65536+32768;CHECK(TryMove(&test));
    test.x=guard->x-MINACTORDIST;test.y=guard->y;CHECK(!TryMove(&test));
    test.x-=1;CHECK(TryMove(&test));
    GameStop();CHECK(!objcount && !statcount && !player.next && !CA_Map() && !ChunksInFile);
}
static void TestSprites(void)
{
    CHECK(GameStart(0));objtype *guard=Guard();CHECK(guard);
    TransformActor(guard);CHECK(guard->viewheight>0 && guard->viewx==VIEWWIDTH/2 && guard->transx>2*65536);
    CHECK(guard->flags&FL_VISABLE);CHECK(CalcRotate(guard)==0);
    guard->dir=east;CHECK(CalcRotate(guard)==4);guard->dir=west;
    guard->state=&s_grdpain;CHECK(CalcRotate(guard)==0);guard->dir=east;CHECK(CalcRotate(guard)==4);
    guard->state=&s_grdstand;guard->x=player.x-65536;TransformActor(guard);CHECK(!guard->viewheight);
    int x,height;CHECK(!TransformTile(5,2,&x,&height) && height>0 && x==VIEWWIDTH/2);
    player.x+=20000;CHECK(TransformTile(3,2,&x,&height));
    memset(framebuffer,201,VL_PIXELS);memset(wallheight,0,sizeof(wallheight));
    CHECK(ScaleShape(120,0,256));
    CHECK(framebuffer[(VIEWY+60)*320+VIEWX+119]==100);
    CHECK(framebuffer[(VIEWY+60)*320+VIEWX+118]==201); /* transparency */
    CHECK(framebuffer[(VIEWY+59)*320+VIEWX+119]==201);
    wallheight[119]=256;memset(framebuffer,201,VL_PIXELS);CHECK(ScaleShape(120,0,256));
    CHECK(framebuffer[(VIEWY+60)*320+VIEWX+119]==201); /* equality occludes */
    CHECK(SimpleScaleShape(120,0,64));CHECK(framebuffer[(VIEWY+60)*320+VIEWX+119]==100);
    /* Equal screen positions must be sorted by original projected height. */
    InitObjects();statcount=2;
    statobjlist[0]=(statobj_t){4,2,SPR_STAT_0,dressing,false};
    statobjlist[1]=(statobj_t){5,2,SPR_STAT_1,dressing,false};
    unsigned char *near=(unsigned char *)PM_GetPage(PMSpriteStart+SPR_STAT_0,NULL);
    unsigned char *far=(unsigned char *)PM_GetPage(PMSpriteStart+SPR_STAT_1,NULL);
    memset(near+14,10,32);memset(far+14,20,32);
    memset(framebuffer,201,VL_PIXELS);memset(wallheight,0,sizeof(wallheight));
    spotvis[4][2]=spotvis[5][2]=true;player.x=2*65536+32768;
    CHECK(DrawScaleds());CHECK(framebuffer[(VIEWY+61)*320+VIEWX+119]==10);
    statobj_t tmp=statobjlist[0];statobjlist[0]=statobjlist[1];statobjlist[1]=tmp;
    memset(framebuffer,201,VL_PIXELS);CHECK(DrawScaleds());CHECK(framebuffer[(VIEWY+61)*320+VIEWX+119]==10);
    memset(near+14,0,32);CHECK(DrawScaleds());CHECK(framebuffer[(VIEWY+61)*320+VIEWX+119]==0); /* opaque index zero */
    /* A nearer wall clips the actual world actor. */
    CHECK(GameStart(0));guard=Guard();for(int y=1;y<63;++y) tilemap[4][y]=1;
    CHECK(ThreeDRefresh());CHECK(!(guard->flags&FL_VISABLE));
    GameStop();
}
static statobj_t Bonus(stat_t item) { return (statobj_t){3,3,SPR_STAT_24,item,true}; }
static void TestPickups(void)
{
    CHECK(GameStart(0));statobj_t ob=Bonus(bo_food);GetBonus(&ob);CHECK(ob.shapenum!=-1 && gamestate.health==100);
    gamestate.health=80;GetBonus(&ob);CHECK(ob.shapenum==-1 && gamestate.health==90);
    ob=Bonus(bo_firstaid);GetBonus(&ob);CHECK(gamestate.health==100);
    ob=Bonus(bo_alpo);gamestate.health=1;GetBonus(&ob);CHECK(gamestate.health==5);
    ob=Bonus(bo_gibs);gamestate.health=11;GetBonus(&ob);CHECK(ob.shapenum!=-1);
    gamestate.health=10;GetBonus(&ob);CHECK(gamestate.health==11 && ob.shapenum==-1);
    gamestate.ammo=99;ob=Bonus(bo_clip);GetBonus(&ob);CHECK(ob.shapenum!=-1);
    gamestate.ammo=94;GetBonus(&ob);CHECK(gamestate.ammo==99 && ob.shapenum==-1);
    gamestate.ammo=0;gamestate.weapon=wp_knife;GiveAmmo(4);CHECK(gamestate.weapon==wp_pistol && gamestate.ammo==4);
    ob=Bonus(bo_machinegun);GetBonus(&ob);CHECK(gamestate.bestweapon==wp_machinegun && gamestate.ammo==10);
    ob=Bonus(bo_chaingun);GetBonus(&ob);CHECK(gamestate.weapon==wp_chaingun && gamestate.ammo==16);
    CHECK(DrawStatus() && framebuffer[164*320+17*8]==GOTGATLINGPIC);
    ob=Bonus(bo_crown);GetBonus(&ob);CHECK(gamestate.score==5000 && gamestate.treasurecount==1);
    GivePoints(35000);CHECK(gamestate.lives==4 && gamestate.nextextra==80000);
    ob=Bonus(bo_key1);GetBonus(&ob);CHECK(gamestate.keys==1);
    doorobjlist[0].lock=1;OperateDoor(0);CHECK(doorobjlist[0].action==dr_opening);
    /* Rendering uses the original projected getting distance, not tile equality. */
    CHECK(GameStart(0));player.x=3*65536+32768;player.y=2*65536+52768;player.tilex=3;player.angle=270;
    gamestate.health=80;CHECK(ThreeDRefresh());CHECK(gamestate.health==90);
    GameStop();
}
static void TestSightStates(void)
{
    CHECK(GameStart(0));objtype *guard=Guard();CHECK(CheckLine(guard) && CheckSight(guard));
    guard->dir=east;CHECK(!CheckSight(guard));guard->dir=west;
    tilemap[4][2]=1;CHECK(!CheckLine(guard) && !CheckSight(guard));
    US_InitRndT(0);CHECK(!SightPlayer(guard) && !guard->temp2);
    madenoise=true;CHECK(!SightPlayer(guard) && guard->temp2==3);madenoise=false;
    CHECK(!SightPlayer(guard));CHECK(!SightPlayer(guard));CHECK(SightPlayer(guard));
    CHECK(guard->state==&s_grdchase1 && guard->speed==SPDPATROL*3 && (guard->flags&FL_ATTACKMODE));
    tilemap[4][2]=0;guard->x=player.x+65536;guard->y=player.y;guard->tilex=3;guard->tiley=2;
    US_InitRndT(0);T_Chase(guard);CHECK(guard->state==&s_grdshoot1);
    /* The shoot action runs after 20+20 tics, on leaving shoot2. */
    int health=gamestate.health;NewState(guard,&s_grdshoot1);
    for(unsigned i=0;i<39;++i) DoActor(guard);
    CHECK(gamestate.health==health && guard->state==&s_grdshoot2);
    US_InitRndT(0);DoActor(guard);CHECK(gamestate.health==health-27);
    guard->x=5*65536+32768;guard->tilex=5;tilemap[4][2]=1;health=gamestate.health;US_InitRndT(0);T_Shoot(guard);CHECK(gamestate.health==health);tilemap[4][2]=0;
    /* Ambush ignores hearing; disconnected areas reject activation. */
    CHECK(GameStart(0));guard=Guard();guard->flags|=FL_AMBUSH;tilemap[4][2]=1;madenoise=true;
    CHECK(!SightPlayer(guard) && !guard->temp2);
    tilemap[4][2]=0;areabyplayer[guard->areanumber]=false;CHECK(!SightPlayer(guard));
    GameStop();
}
static void TestMovement(void)
{
    CHECK(GameStart(0));objtype *guard=Guard();
    guard->dir=southwest;tilemap[guard->tilex-1][guard->tiley]=1;CHECK(!TryWalk(guard));tilemap[guard->tilex-1][guard->tiley]=0;
    guard->dir=east;CHECK(TryWalk(guard) && guard->tilex==6 && guard->distance==-1 && doorobjlist[0].action==dr_opening);
    /* Door occupancy includes the reserved destination and physical bounds. */
    doorobjlist[0].position=65535;doorobjlist[0].action=dr_open;CloseDoor(0);CHECK(doorobjlist[0].action==dr_open);
    CHECK(GameStart(0));guard=Guard();guard->x=player.x+MINACTORDIST+100;guard->y=player.y;guard->dir=west;guard->distance=65536;
    int32_t x=guard->x;MoveObj(guard,200);CHECK(guard->x==x && guard->distance==65536);
    MoveObj(guard,50);CHECK(guard->x==x-50 && guard->distance==65486);
    CHECK(GameStart(0));
    objtype *dog=&objlist[1];dog->x=player.x+65536;dog->y=player.y;
    dog->dir=west;dog->distance=65536;T_DogChase(dog);CHECK(dog->state==&s_dogjump1);
    int health=gamestate.health;US_InitRndT(0);T_Bite(dog);CHECK(gamestate.health==health-6);
    CHECK(GameStart(0));
    dog=&objlist[1];dog->flags|=FL_AMBUSH;dog->dir=west;
    int32_t original=dog->x;DoActor(dog);CHECK(dog->x==original-SPDDOG);
    /* A map arrow replaces the current patrol direction, reserving its cell. */
    actorat[dog->tilex][dog->tiley]=NULL;dog->tilex=26;dog->tiley=1;dog->dir=east;
    SelectPathDir(dog);CHECK(dog->tilex==25 && dog->tiley==1 && dog->dir==west);
    GameStop();
}
static void TestAreas(void)
{
    CHECK(GameStart(0));areaMap[2][2]=areaMap[5][2]=1;areaMap[7][2]=2;
    ConnectAreas();CHECK(areabyplayer[1] && !areabyplayer[2]);
    OpenDoor(0);ConnectAreas();CHECK(!areabyplayer[2]);
    MoveDoors();ConnectAreas();CHECK(areabyplayer[2]);
    doorobjlist[0].action=dr_closing;MoveDoors();ConnectAreas();CHECK(!areabyplayer[2]);
    GameStop();
}
static void TestCombat(void)
{
    CHECK(GameStart(0));objtype *guard=Guard();US_InitRndT(0);DamageActor(guard,3);
    CHECK(guard->hitpoints==19 && (guard->state==&s_grdpain || guard->state==&s_grdpain1));
    CHECK(guard->flags&FL_ATTACKMODE);DamageActor(guard,3);CHECK(guard->hitpoints==16);
    DamageActor(guard,100);CHECK(guard->state==&s_grddie1 && !(guard->flags&FL_SHOOTABLE));
    CHECK(gamestate.score==100 && gamestate.killcount==1 && statcount==9 && statobjlist[8].itemnumber==bo_clip2);
    for(unsigned i=0;i<45;++i) { DoActor(guard); }
    CHECK(guard->state==&s_grddie4);
    CHECK(GameStart(0));guard=Guard();
    T_Attack(true);CHECK(gamestate.weaponframe==1 && gamestate.ammo==8);
    for(unsigned t=0;t<11;++t) T_Attack(true);
    CHECK(gamestate.ammo==8);US_InitRndT(0);T_Attack(true);
    CHECK(gamestate.ammo==7 && guard->hitpoints==23); /* RNG 8 / 6, doubled surprise. */
    for(unsigned t=0;t<12;++t) T_Attack(true);
    CHECK(!gamestate.attackframe);for(unsigned t=0;t<50;++t) T_Attack(true);CHECK(gamestate.ammo==7); /* pistol is semi-auto */
    T_Attack(false);T_Attack(true);for(unsigned t=0;t<12;++t) T_Attack(true);CHECK(gamestate.ammo==6);
    CHECK(GameStart(0));GiveWeapon(wp_machinegun);int ammo=gamestate.ammo;
    T_Attack(true);for(unsigned t=0;t<36;++t) T_Attack(true);CHECK(gamestate.ammo==ammo-3);
    CHECK(GameStart(0));GiveWeapon(wp_chaingun);ammo=gamestate.ammo;
    T_Attack(true);for(unsigned t=0;t<36;++t) T_Attack(true);CHECK(gamestate.ammo==ammo-5);
    CHECK(GameStart(0));gamestate.ammo=1;T_Attack(true);for(unsigned t=0;t<24;++t) T_Attack(true);CHECK(gamestate.ammo==0 && gamestate.weapon==wp_knife);
    IN_KeyEvent(0x02,true);T_Attack(false);CHECK(gamestate.weapon==wp_knife);IN_ClearKeysDown();
    TakeDamage(1,Guard());CHECK(gamestate.health==99 && !gamestate.died);TakeDamage(100,Guard());CHECK(gamestate.health==0 && gamestate.died);
    CHECK(GameStart(0));guard=Guard();guard->obclass=ssobj;DamageActor(guard,1000);
    CHECK(statobjlist[statcount-1].itemnumber==bo_machinegun && gamestate.score==500);
    CHECK(GameStart(0));guard=Guard();guard->x=player.x+65536;guard->tilex=3;
    CHECK(ThreeDRefresh());gamestate.weapon=wp_knife;US_InitRndT(0);
    T_Attack(true);for(unsigned t=0;t<12;++t) T_Attack(true);CHECK(gamestate.ammo==8 && guard->hitpoints==25); /* 8 >> 4 = 0 */
    CHECK(GameStart(0));GiveWeapon(wp_chaingun);IN_KeyEvent(0x03,true);T_Attack(false);
    CHECK(gamestate.weapon==wp_pistol && gamestate.chosenweapon==wp_pistol);IN_ClearKeysDown();
    gamestate.ammo=0;gamestate.weapon=wp_knife;IN_KeyEvent(0x05,true);T_Attack(false);CHECK(gamestate.weapon==wp_knife);IN_ClearKeysDown();
    CHECK(GameStart(0));GiveWeapon(wp_chaingun);
    IN_KeyEvent(0x02,true);IN_KeyEvent(0x05,true);T_Attack(false);
    CHECK(gamestate.weapon==wp_knife && gamestate.chosenweapon==wp_knife);IN_ClearKeysDown();
    gamestate.chosenweapon=wp_pistol;gamestate.ammo=0;
    IN_KeyEvent(0x02,true);T_Attack(false);CHECK(gamestate.chosenweapon==wp_pistol);IN_ClearKeysDown();
    GiveAmmo(8);CHECK(gamestate.weapon==wp_pistol);
    GameStop();
}
static void TestHud(void)
{
    CHECK(GameStart(0));CHECK(framebuffer[176*320+14*8]==N_0PIC+3);
    CHECK(framebuffer[176*320+23*8]==N_0PIC);CHECK(framebuffer[176*320+28*8]==N_0PIC+8);
    gamestate.health=7;gamestate.ammo=42;gamestate.score=123;gamestate.keys=1;gamestate.weapon=wp_machinegun;
    CHECK(DrawStatus());CHECK(framebuffer[176*320+23*8]==N_0PIC+7);
    CHECK(framebuffer[176*320+27*8]==N_0PIC+4 && framebuffer[176*320+28*8]==N_0PIC+2);
    CHECK(framebuffer[164*320+30*8]==GOLDKEYPIC);CHECK(framebuffer[168*320+32*8]==MACHINEGUNPIC);
    CHECK(framebuffer[164*320+17*8]==FACE1APIC+3*5+gamestate.faceframe);
    US_InitRndT(0);int first=-1;
    for(unsigned i=0;i<256;++i) { UpdateFace();CHECK(gamestate.faceframe>=0 && gamestate.faceframe<=2);if(gamestate.faceframe) first=1; }
    CHECK(first==1);
    GameStop();
}
static void TestTicsDeath(void)
{
    CHECK(GameStart(0));IN_KeyEvent(sc_UpArrow,true);CHECK(GameStep(9));
    objtype fine[MAXACTORS];gametype state=gamestate;memcpy(fine,objlist,sizeof(fine));
    CHECK(GameStart(0));IN_KeyEvent(sc_UpArrow,true);for(uint64_t t=1;t<=9;++t) CHECK(GameStep(t));
    CHECK(gamestate.health==state.health && gamestate.ammo==state.ammo && gamestate.faceframe==state.faceframe);
    for(unsigned i=0;i<objcount;++i) CHECK(objlist[i].x==fine[i].x && objlist[i].y==fine[i].y && objlist[i].state==fine[i].state && objlist[i].temp2==fine[i].temp2);
    CHECK(GameStart(0));uint64_t ticks=0;
    for(int life=3;life>=0;--life) {
        CHECK(gamestate.lives==life);TakeDamage(100,Guard());
        unsigned steps=0;while(gamestate.died && !GameFinished() && steps++<500) CHECK(GameStep(++ticks));
        CHECK(steps<500);
        if(life) CHECK(!GameFinished() && gamestate.health==100 && gamestate.ammo==8 && gamestate.lives==life-1);
    }
    CHECK(GameFinished() && gamestate.lives==-1);
    GameStop();
}
static void TestFlashes(void)
{
    CHECK(GameStart(0));StartBonusFlash();CHECK(UpdatePaletteShifts());
    for(unsigned j=0;j<768;++j) {
        int base=gamepal[j],target=j%3==0?64:j%3==1?62:0;
        CHECK(palette[j]==base+(target-base)*3/20);
    }
    StartDamageFlash(10);CHECK(UpdatePaletteShifts());
    for(unsigned j=0;j<768;++j) {
        int base=gamepal[j],target=j%3==0?64:0;
        CHECK(palette[j]==base+(target-base)*2/8);
    }
    for(unsigned i=0;i<20;++i) CHECK(UpdatePaletteShifts());
    CHECK(!memcmp(palette,gamepal,768));GameStop();
}
static void TestMalformedSprites(void)
{
    CHECK(GameStart(0));
    unsigned char *page=(unsigned char *)PM_GetPage(PMSpriteStart,NULL),saved[46];
    memcpy(saved,page,46);
    unsigned char pixels[4098],mask[4098];uint32_t seed=5;
    for(unsigned attempt=0;attempt<1000;++attempt) {
        memcpy(page,saved,46);seed=seed*1664525U+1013904223U;
        page[seed%46]^=(unsigned char)(seed>>24);
        memset(pixels,0x57,sizeof(pixels));memset(mask,0x57,sizeof(mask));
        (void)PM_DecodeSprite(0,pixels+1,mask+1);
        CHECK(pixels[0]==0x57 && pixels[4097]==0x57 && mask[0]==0x57 && mask[4097]==0x57);
    }
    memcpy(page,saved,46);
    /* Asymmetric command columns must use the original right-half anchor. */
    page[0]=43;page[2]=43;memset(framebuffer,201,VL_PIXELS);
    CHECK(SimpleScaleShape(120,0,64));
    CHECK(framebuffer[(VIEWY+60)*320+VIEWX+120]==100);
    CHECK(framebuffer[(VIEWY+60)*320+VIEWX+131]==201);
    memcpy(page,saved,46);GameStop();
}
static void TestGameOverMenu(void)
{
    CHECK(DemoLoopStartGame(0));CHECK(DemoLoop(10));gamestate.lives=0;TakeDamage(100,Guard());
    uint64_t ticks=10;
    while(DemoLoopState()==UI_GAME && ticks<600) CHECK(DemoLoop(++ticks));
    CHECK(DemoLoopState()==UI_SCORES && !CA_Map() && !ChunksInFile);
    CHECK(DemoLoop(ticks+10));IN_KeyEvent(sc_Enter,true);IN_KeyEvent(sc_Enter,false);
    CHECK(DemoLoop(ticks+11) && DemoLoop(ticks+21) && DemoLoopState()==UI_MENU);
}
static void TestOwnedCombat(void)
{
    CHECK(GameStart(0));CHECK(objcount==21 && statcount==121 && gamestate.killtotal==20 && gamestate.treasuretotal==23);
    objtype *guard=NULL;
    for(unsigned i=0;i<objcount && !guard;++i) if(objlist[i].obclass==guardobj) {
        objtype *ob=&objlist[i];
        const int dx[4]={-2,2,0,0},dy[4]={0,0,-2,2},angle[4]={0,180,270,90};
        for(unsigned d=0;d<4;++d) {
            player.tilex=ob->tilex+dx[d];player.tiley=ob->tiley+dy[d];
            player.x=player.tilex*65536+32768;player.y=player.tiley*65536+32768;player.angle=angle[d];
            if(TryMove(&player) && CheckLine(ob)) { guard=ob;break; }
        }
    }
    CHECK(guard && ThreeDRefresh() && (guard->flags&FL_VISABLE));
    uint64_t ticks=0;
    /* Aim with the actual angle and use real input/tics; no damage override. */
    for(unsigned shot=0;shot<8 && (guard->flags&FL_SHOOTABLE) && !gamestate.died;++shot) {
        IN_KeyEvent(0x1d,true);
        for(unsigned j=0;j<25;++j) {
            double a=atan2((double)player.y-guard->y,(double)guard->x-player.x);
            if(a<0) a+=6.28318530717958647692;
            player.angle=(int)(a/6.28318530717958647692*360);
            CHECK(GameStep(++ticks));
        }
        IN_KeyEvent(0x1d,false);CHECK(GameStep(++ticks));
    }
    CHECK(!(guard->flags&FL_SHOOTABLE) && gamestate.killcount>0 && gamestate.score>=100 && gamestate.ammo<8);
    GameStop();
}
int main(int argc,char **argv)
{
    CHECK(argc==2 || argc==3);MM_Startup();CHECK(CA_Startup(argv[1]));CHECK(VL_SetPalette(gamepal,768));
    if(argc==3) TestOwnedCombat();
    else { TestRandom();TestSpawn();TestSprites();TestPickups();TestSightStates();TestMovement();TestAreas();TestCombat();TestHud();TestTicsDeath();TestFlashes();TestMalformedSprites();TestGameOverMenu(); }
    GameStop();CA_Shutdown();MM_Shutdown();CHECK(!MM_TotalBytes());
    puts("Original sprite/object/RNG/AI/combat/HUD/death tests passed.");return 0;
}
