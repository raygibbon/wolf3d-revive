/* SPDX-License-Identifier: GPL-2.0-only */
#include "engine/WL_WORKSHOP.h"
#include "engine/WL_ACTOR.h"
#include "engine/WL_RENDER.h"
#include "engine/WL_MAIN.h"
#include "engine/WL_MENU.h"
#include "engine/WL_LEVEL.h"
#include "engine/WL_SAVE.h"
#include "engine/ID_CA.h"
#include "engine/ID_IN.h"
#include "engine/ID_MM.h"
#include "engine/ID_PM.h"
#include "engine/ID_VL.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHECK(x) do { if(!(x)) { fprintf(stderr,"%s:%d: %s\n",__FILE__,__LINE__,#x);exit(1); } } while(0)

static w3r_byte tiles[64][64];
static objtype *actors[64][64];
static bool blocks[64][64],pushables[64][64];
static objtype subject,occupant;
static statobj_t statics[2];
static void PrepareOverlayWorld(void);

static wl_edit_world Fixture(void)
{
    memset(tiles,0,sizeof(tiles));memset(actors,0,sizeof(actors));
    memset(blocks,0,sizeof(blocks));memset(pushables,0,sizeof(pushables));
    memset(&subject,0,sizeof(subject));memset(&occupant,0,sizeof(occupant));memset(statics,0,sizeof(statics));
    for(unsigned i=0;i<64;++i) tiles[0][i]=tiles[63][i]=tiles[i][0]=tiles[i][63]=1;
    subject.x=10*TILEGLOBAL+TILEGLOBAL/2;subject.y=10*TILEGLOBAL+TILEGLOBAL/2;subject.tilex=subject.tiley=10;
    statics[0].shapenum=-1;statics[1].shapenum=-1;
    return (wl_edit_world){tiles,actors,blocks,pushables,statics,2,&subject,64,64,63,0,0,0};
}

static void TestEditing(void)
{
    wl_edit_world world=Fixture();tiles[2][2]=7;tiles[30][30]=9;
    CHECK(WL_EditSetWall(&world,12,10,4)==WL_EDIT_OK && tiles[12][10]==4);
    CHECK(tiles[2][2]==7 && tiles[30][30]==9);
    CHECK(WL_EditSetWall(&world,12,10,4)==WL_EDIT_OCCUPIED);
    CHECK(WL_EditRemoveWall(&world,12,10)==WL_EDIT_OK && !tiles[12][10]);
    CHECK(WL_EditRemoveWall(&world,12,10)==WL_EDIT_NOT_WALL);
    CHECK(WL_EditSetWall(&world,12,10,4)==WL_EDIT_OK);
    CHECK(WL_EditRemoveWall(&world,12,10)==WL_EDIT_OK);
    CHECK(WL_EditSetWall(&world,11,10,4)==WL_EDIT_OK);
    CHECK(WL_EditSetWall(&world,-1,2,4)==WL_EDIT_OUT_OF_BOUNDS);
    CHECK(WL_EditSetWall(&world,2,-1,4)==WL_EDIT_OUT_OF_BOUNDS);
    CHECK(WL_EditSetWall(&world,64,2,4)==WL_EDIT_OUT_OF_BOUNDS);
    CHECK(WL_EditSetWall(&world,2,64,4)==WL_EDIT_OUT_OF_BOUNDS);
    CHECK(WL_EditSetWall(&world,12,10,0)==WL_EDIT_INVALID_WALL);
    CHECK(WL_EditSetWall(&world,12,10,64)==WL_EDIT_INVALID_WALL);
    CHECK(WL_EditSetWall(&world,12,10,21)==WL_EDIT_INVALID_WALL);
    CHECK(WL_EditSetWall(&world,10,10,4)==WL_EDIT_OCCUPIED);
    actors[12][10]=&occupant;CHECK(WL_EditSetWall(&world,12,10,4)==WL_EDIT_OCCUPIED);actors[12][10]=NULL;
    statics[0].shapenum=1;statics[0].tilex=12;statics[0].tiley=10;
    CHECK(WL_EditSetWall(&world,12,10,4)==WL_EDIT_OCCUPIED);statics[0].shapenum=-1;
    tiles[12][10]=128;CHECK(WL_EditRemoveWall(&world,12,10)==WL_EDIT_NOT_WALL);
    tiles[12][10]=21;CHECK(WL_EditRemoveWall(&world,12,10)==WL_EDIT_PROTECTED);
    tiles[0][4]=3;CHECK(WL_EditRemoveWall(&world,0,4)==WL_EDIT_PROTECTED && tiles[0][4]==3);
    tiles[12][10]=3;pushables[12][10]=true;
    CHECK(WL_EditRemoveWall(&world,12,10)==WL_EDIT_PROTECTED && tiles[12][10]==3);
    pushables[12][10]=false;tiles[2][4]=3;tiles[0][4]=tiles[1][4]=0;
    CHECK(WL_EditRemoveWall(&world,2,4)==WL_EDIT_PROTECTED && tiles[2][4]==3);
    tiles[9][10]=tiles[10][9]=tiles[10][11]=2;tiles[11][10]=0;
    CHECK(WL_EditSetWall(&world,11,10,4)==WL_EDIT_PROTECTED && !tiles[11][10]);
    CHECK(WL_EditSetWall(NULL,1,1,1)==WL_EDIT_INVALID_STATE);
    subject.tilex=64;CHECK(WL_EditSetWall(&world,2,2,1)==WL_EDIT_INVALID_STATE);
}

static void TestAtomicFailure(void)
{
    wl_edit_world world=Fixture();tiles[12][10]=5;
    w3r_byte before[64][64];memcpy(before,tiles,sizeof(before));
    CHECK(WL_EditSetWall(&world,12,10,6)==WL_EDIT_OCCUPIED);
    CHECK(!memcmp(before,tiles,sizeof(before)));
    tiles[12][10]=128;memcpy(before,tiles,sizeof(before));
    CHECK(WL_EditRemoveWall(&world,12,10)==WL_EDIT_NOT_WALL);
    CHECK(!memcmp(before,tiles,sizeof(before)));
}

static void TestSelection(void)
{
    BuildTables();wl_edit_world world=Fixture();wl_workshop_target target;
    tiles[13][10]=2;
    CHECK(WL_WorkshopSelect(&world,subject.x,subject.y,0,4,&target));
    CHECK(target.remove_x==13 && target.remove_y==10 && target.place_x==12 && target.place_y==10);
    CHECK(target.place_result==WL_EDIT_OK && target.remove_result==WL_EDIT_OK);
    tiles[13][10]=0;tiles[12][8]=2;
    CHECK(WL_WorkshopSelect(&world,subject.x,subject.y,45,4,&target));
    CHECK(target.remove_x==12 && target.remove_y==8 && target.place_x==11 && target.place_y==9);
    tiles[12][8]=0;tiles[13][10]=2;
    CHECK(WL_WorkshopSelect(&world,subject.x,subject.y,0,4,&target)); /* 2.5 tiles */
    tiles[13][10]=0;tiles[14][10]=2;
    CHECK(WL_WorkshopSelect(&world,subject.x,subject.y,0,4,&target)); /* free cell before 3.5-tile wall */
    CHECK(!target.hit && target.place_x==11 && target.place_y==10);
    tiles[14][10]=0;tiles[12][10]=2;subject.x=10*TILEGLOBAL;
    for(unsigned i=0;i<20;++i) {
        CHECK(WL_WorkshopSelect(&world,subject.x,subject.y,0,4,&target));
        CHECK(target.remove_x==12 && target.place_x==11);
    }
    /* Free placement follows the first crossed grid boundary, even when the
       player is near the rear edge of the current tile. */
    tiles[12][10]=0;subject.x=10*TILEGLOBAL+100;subject.y=10*TILEGLOBAL+TILEGLOBAL/2;
    subject.tilex=subject.tiley=10;
    CHECK(WL_WorkshopSelect(&world,subject.x,subject.y,0,4,&target));
    CHECK(!target.hit && target.has_place && target.place_x==11 && target.place_y==10);
    CHECK(target.place_result==WL_EDIT_OK);
    subject.x=TILEGLOBAL+TILEGLOBAL/2;subject.y=TILEGLOBAL+TILEGLOBAL/2;
    tiles[0][1]=0;
    CHECK(!WL_WorkshopSelect(&world,subject.x,subject.y,180,4,&target));
}

static void TestDisconnectedRoomsValidate(void)
{
    wl_level level;char error[128];CHECK(WL_LevelBlank(&level,1));
    for(unsigned x=5;x<=9;++x) level.tiles[x][5]=level.tiles[x][9]=2;
    for(unsigned y=5;y<=9;++y) level.tiles[5][y]=level.tiles[9][y]=2;
    CHECK(WL_LevelValidate(&level,error,sizeof(error)));
    level.tiles[0][7]=0;CHECK(!WL_LevelValidate(&level,error,sizeof(error)));
    CHECK(strstr(error,"boundary")!=NULL);
}

static void TestPickerAssignment(void)
{
    PrepareOverlayWorld();WL_WorkshopSetEnabled(true);
    unsigned ids[12];for(unsigned i=0;i<12;++i) ids[i]=i+1;
    CHECK(WL_WorkshopSetMaterials(ids,12));
    CHECK(WL_WorkshopSelectSlot(2));CHECK(WL_WorkshopSetSlotMaterial(2,12));
    WL_WorkshopPickerToggle();CHECK(WL_WorkshopPickerOpen() && WL_WorkshopPickerPage()==1);
    IN_KeyEvent(0x02,true);IN_KeyEvent(0x02,false);WL_WorkshopTick();
    CHECK(!WL_WorkshopPickerOpen());CHECK(WL_WorkshopSelectedSlot()==2);
    CHECK(WL_WorkshopHotbarMaterial(2)==10);
    IN_KeyEvent(0x03,true);IN_KeyEvent(0x03,false);WL_WorkshopTick();
    CHECK(WL_WorkshopSelectedSlot()==1);
    WL_WorkshopSetEnabled(false);
}

static void BuildRoom(wl_edit_world *world,int left,int top,int right,int bottom,unsigned material)
{
    for(int x=left;x<=right;++x) {
        CHECK(WL_EditSetWall(world,x,top,material)==WL_EDIT_OK);
        CHECK(WL_EditSetWall(world,x,bottom,material)==WL_EDIT_OK);
    }
    for(int y=top+1;y<bottom;++y) {
        CHECK(WL_EditSetWall(world,left,y,material)==WL_EDIT_OK);
        CHECK(WL_EditSetWall(world,right,y,material)==WL_EDIT_OK);
    }
}

static void Tap(unsigned scan) { IN_KeyEvent(scan,true);IN_KeyEvent(scan,false); }

static void TestAuthoredLifecycle(const char *data_directory,const char *save_directory)
{
    MM_Startup();CHECK(CA_Startup(data_directory));CHECK(Save_SetDirectory(save_directory));
    /* New Level must pass through UI_GAME's authored startup, not campaign
       NewGame(). */
    CHECK(DemoLoopStartup(0,true));CHECK(DemoLoop(10));
    MainItems.curpos=newlevel;Tap(sc_Enter);
    CHECK(DemoLoop(11) && DemoLoop(31) && DemoLoopState()==UI_GAME);
    CHECK(GameRunning() && GameAuthoredLevel() && WL_WorkshopEnabled());
    CHECK(player.tilex==32 && player.tiley==32 && player.angle==0);
    GamePreloadConsumed();

    /* Give this authored definition a non-cardinal spawn to prove metadata
       survives a complete install/start/save/load cycle. */
    wl_level level;CHECK(GameCaptureAuthoredLevel(&level));level.spawn_angle=37;
    CHECK(GameInstallAuthoredLevel(&level));CHECK(GameStartAuthored(100,2,42));
    CHECK(player.angle==37 && WL_WorkshopEnabled());GamePreloadConsumed();

    wl_edit_world world=WL_WorkshopLiveWorld();
    BuildRoom(&world,8,8,12,12,2);BuildRoom(&world,18,8,23,13,2);
    char error[128];CHECK(GameSaveAuthoredLevel(save_directory,error,sizeof(error)));
    wl_level saved;CHECK(WL_LevelLoad(&saved,save_directory,error,sizeof(error)));
    CHECK(saved.spawn_x==32 && saved.spawn_y==32 && saved.spawn_angle==37);
    CHECK(saved.tiles[8][8]==2 && saved.tiles[12][12]==2);
    CHECK(saved.tiles[18][8]==2 && saved.tiles[23][13]==2);

    GameStop();GameClearAuthoredLevel();CHECK(!GameAuthoredLevel());
    /* Load Level stages the file and enters through the same fresh authored
       game lifecycle. It must not pretend an old game is being resumed. */
    CHECK(DemoLoopStartup(200,true));CHECK(DemoLoop(210));
    MainItems.curpos=loadlevel;Tap(sc_Enter);
    CHECK(DemoLoop(211) && DemoLoop(231) && DemoLoopState()==UI_GAME);
    CHECK(GameAuthoredLevel() && WL_WorkshopEnabled());GamePreloadConsumed();
    CHECK(tilemap[8][8]==2 && tilemap[12][12]==2 && tilemap[18][8]==2 && tilemap[23][13]==2);
    CHECK(player.angle==37);
    world=WL_WorkshopLiveWorld();CHECK(WL_EditSetWall(&world,25,25,2)==WL_EDIT_OK);

    /* Ordinary campaign startup remains independent and clears the staged
       authored definition. */
    CHECK(GameStart(300));CHECK(!GameAuthoredLevel());
    CHECK(player.tilex==2 && player.tiley==2);
    GameStop();CA_Shutdown();MM_Shutdown();CHECK(!MM_TotalBytes());
}

static void TestProductionCollision(void)
{
    memset(tilemap,0,sizeof(tilemap));memset(actorat,0,sizeof(actorat));memset(staticblock,0,sizeof(staticblock));
    memset(pushable,0,sizeof(pushable));memset(statobjlist,0,sizeof(statobjlist));statcount=0;pwallstate=0;
    player.x=10*TILEGLOBAL+TILEGLOBAL/2;player.y=10*TILEGLOBAL+TILEGLOBAL/2;player.tilex=player.tiley=10;
    player.angle=0;doornum=0;PMSpriteStart=128;BuildTables();
    for(unsigned i=0;i<64;++i) tilemap[0][i]=tilemap[63][i]=tilemap[i][0]=tilemap[i][63]=1;
    objtype mover={0};mover.x=12*TILEGLOBAL+TILEGLOBAL/2;mover.y=10*TILEGLOBAL+TILEGLOBAL/2;
    wl_edit_world world=WL_WorkshopLiveWorld();
    rayhit hit;
    CHECK(TryMove(&mover));
    CHECK(WL_EditSetWall(&world,12,10,3)==WL_EDIT_OK);
    CHECK(WL_EditTileBlocked(&world,12,10) && !TryMove(&mover));
    CHECK(WL_RenderRay(0,&hit) && hit.tile==3);
    CHECK(WL_EditRemoveWall(&world,12,10)==WL_EDIT_OK);
    CHECK(!WL_EditTileBlocked(&world,12,10) && TryMove(&mover));
    CHECK(WL_RenderRay(0,&hit) && hit.tile==1);
}

static void PrepareOverlayWorld(void)
{
    memset(tilemap,0,sizeof(tilemap));memset(actorat,0,sizeof(actorat));memset(staticblock,0,sizeof(staticblock));
    memset(pushable,0,sizeof(pushable));memset(statobjlist,0,sizeof(statobjlist));statcount=0;pwallstate=0;
    player.x=10*TILEGLOBAL+TILEGLOBAL/2;player.y=10*TILEGLOBAL+TILEGLOBAL/2;player.tilex=player.tiley=10;
    player.angle=0;doornum=0;PMSpriteStart=128;BuildTables();
    for(unsigned i=0;i<64;++i) tilemap[0][i]=tilemap[63][i]=tilemap[i][0]=tilemap[i][63]=1;
    tilemap[13][10]=2;
    WL_WorkshopReset();
    CHECK(NewViewSize(15));
}

static void PatternFramebuffer(void)
{
    for(unsigned y=0;y<200;++y) for(unsigned x=0;x<320;++x)
        framebuffer[y*320+x]=(w3r_byte)((x*3U+y*5U)&0xffU);
}

static void CheckOverlayInsideViewport(const w3r_byte *baseline)
{
    bool changed=false;
    for(unsigned y=0;y<200;++y) for(unsigned x=0;x<320;++x) {
        if(framebuffer[y*320+x]==baseline[y*320+x]) continue;
        changed=true;
        CHECK(x>=(unsigned)VIEWX && x<(unsigned)VIEWX+(unsigned)VIEWWIDTH);
        CHECK(y>=(unsigned)VIEWY && y<(unsigned)VIEWY+(unsigned)VIEWHEIGHT);
    }
    CHECK(changed);
}

static void TestOverlayCleanup(void)
{
    PrepareOverlayWorld();
    static w3r_byte baseline[320*200];
    PatternFramebuffer();memcpy(baseline,framebuffer,sizeof(baseline));
    WL_WorkshopSetEnabled(true);CHECK(WL_WorkshopEnabled());
    WL_WorkshopDrawClassicOverlay();
    CHECK(!memcmp(framebuffer+160*320,baseline+160*320,40*320));
    CheckOverlayInsideViewport(baseline);
    WL_WorkshopClearOverlay();
    CHECK(!memcmp(framebuffer,baseline,sizeof(baseline)));
    WL_WorkshopDrawClassicOverlay();
    WL_WorkshopSetEnabled(false);CHECK(!WL_WorkshopEnabled());
    CHECK(!memcmp(framebuffer,baseline,sizeof(baseline)));
    WL_WorkshopDrawClassicOverlay();
    CHECK(!memcmp(framebuffer,baseline,sizeof(baseline)));
    WL_WorkshopSetEnabled(true);
    WL_WorkshopDrawClassicOverlay();
    WL_WorkshopReset();CHECK(!WL_WorkshopEnabled());
    CHECK(!memcmp(framebuffer,baseline,sizeof(baseline)));
}

static void TestOverlayViewSizes(void)
{
    PrepareOverlayWorld();
    static w3r_byte baseline[320*200];
    for(unsigned size=4;size<=19;++size) {
        CHECK(NewViewSize(size));
        PatternFramebuffer();memcpy(baseline,framebuffer,sizeof(baseline));
        WL_WorkshopSetEnabled(true);
        WL_WorkshopDrawClassicOverlay();
        CHECK(!memcmp(framebuffer+160*320,baseline+160*320,40*320));
        CheckOverlayInsideViewport(baseline);
        WL_WorkshopClearOverlay();
        CHECK(!memcmp(framebuffer,baseline,sizeof(baseline)));
        WL_WorkshopSetEnabled(false);
    }
    CHECK(NewViewSize(15));
}

static void TestTargetOverlay(void)
{
    enum { WIDTH=640,HEIGHT=480 };
    PrepareOverlayWorld();
    PatternFramebuffer();
    WL_WorkshopSetEnabled(true);
    WL_WorkshopDrawClassicOverlay();
    static w3r_byte pixels[WIDTH*HEIGHT];
    wl_render_target target;
    memset(pixels,0x22,sizeof(pixels));
    CHECK(WL_TargetInit(&target,pixels,sizeof(pixels),WIDTH,HEIGHT,WIDTH));
    static w3r_byte before[WIDTH*HEIGHT];
    memcpy(before,pixels,sizeof(before));
    int vx=80,vy=48,vw=480,vh=288;
    WL_WorkshopDrawTargetOverlay(&target,vx,vy,vw,vh);
    bool changed=false,panel=false;
    for(unsigned y=0;y<HEIGHT;++y) for(unsigned x=0;x<WIDTH;++x) {
        size_t at=(size_t)y*WIDTH+x;
        if(pixels[at]!=before[at]) {
            changed=true;
            CHECK(x>=(unsigned)vx && x<(unsigned)(vx+vw));
            CHECK(y>=(unsigned)vy && y<(unsigned)(vy+vh));
        }
        if(pixels[at]==0x7f) panel=true;
    }
    CHECK(changed && panel);
    WL_WorkshopClearOverlay();
    memset(pixels,0x22,sizeof(pixels));memcpy(before,pixels,sizeof(before));
    WL_WorkshopDrawTargetOverlay(&target,vx,vy,vw,vh);
    CHECK(!memcmp(pixels,before,sizeof(before)));
    WL_WorkshopSetEnabled(false);
    WL_WorkshopDrawTargetOverlay(&target,vx,vy,vw,vh);
    CHECK(!memcmp(pixels,before,sizeof(before)));
}

int main(int argc,char **argv)
{
    CHECK(argc==3);
    TestEditing();TestAtomicFailure();TestSelection();TestProductionCollision();
    TestDisconnectedRoomsValidate();TestOverlayCleanup();TestOverlayViewSizes();TestTargetOverlay();
    TestPickerAssignment();TestAuthoredLifecycle(argv[1],argv[2]);
    puts("workshop edit/selection/collision/overlay/authored lifecycle tests passed");return 0;
}
