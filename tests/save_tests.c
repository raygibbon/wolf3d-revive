/* SPDX-License-Identifier: GPL-2.0-only */
#include "engine/WL_SAVE.h"
#include "engine/ID_CA.h"
#include "engine/ID_VL.h"
#include "engine/ID_IN.h"
#include "engine/ID_US_1.h"
#include "engine/ID_SD.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(x) do { if(!(x)) { fprintf(stderr,"%d: %s (%s)\n",__LINE__,#x,CA_Error());exit(1); } } while(0)
static void Step(unsigned start,unsigned count)
{
    for(unsigned t=start;t<start+count;++t) {
        IN_KeyEvent(0x1d,t%70<25);IN_KeyEvent(sc_RightArrow,t%13<3);CHECK(GameStep(t));
    }
}
static void Start(void) { CHECK(GameStartSelected(0,0,2)); }
static void Scenario(void)
{
    gamestate.health=89;gamestate.ammo=77;gamestate.lives=7;gamestate.keys=3;
    gamestate.score=45678;gamestate.nextextra=80000;gamestate.oldscore=12345;
    GiveWeapon(wp_chaingun);gamestate.attackframe=2;gamestate.attackcount=4;gamestate.weaponframe=2;
    OpenDoor(0);for(unsigned i=0;i<17;++i) MoveDoors();
    doorobjlist[0].ticcount=304; /* a blocked DOS timer need not stop at 300 */
    tilemap[30][12]=1;pushable[30][12]=true;PushWall(30,12,east);
    for(unsigned i=0;i<37;++i) MovePWalls();
    PlaceItemType(bo_clip,3,3);GetBonus(&statobjlist[statcount-1]);
    staticblock[40][40]=true;
    for(objtype *o=player.next;o;o=o->next) FirstSighting(o);
    T_FakeFire(&objlist[1]);DamageActor(&objlist[1],10000);
    RemoveActor(&objlist[2]);T_GiftThrow(&objlist[3]);T_SchabbThrow(&objlist[0]);
    CHECK(ThreeDRefresh());
    CHECK(pwallstate && doorobjlist[0].action==dr_opening && statobjlist[statcount-1].shapenum==-1);
    CHECK(Save_Write(0,"moving world",true));
    Step(1,90);CHECK(Save_Write(1,"continuous",true));unsigned rng=US_RndIndex();
    CHECK(Save_Load(0,0));CHECK(gamestate.health==89 && gamestate.ammo==91 && gamestate.lives==7 && gamestate.keys==3);
    CHECK(pwallstate==38 && pwallpos==19 && doorobjlist[0].position==17408 && doorobjlist[0].ticcount==304 && staticblock[40][40]);
    CHECK(statobjlist[statcount-1].shapenum==-1 && gamestate.weapon==wp_chaingun && gamestate.attackframe==2);
    Step(1,90);CHECK(US_RndIndex()==rng);CHECK(Save_Write(2,"restored",true));
}
static void End(void) { GameStop();SD_Shutdown();CA_Shutdown();MM_Shutdown();CHECK(!MM_TotalBytes()); }
int main(int argc,char **argv)
{
    CHECK(argc>=3);MM_Startup();CHECK(CA_Startup(argv[1]));CHECK(VL_SetPalette(gamepal,768));CHECK(Save_SetDirectory(argv[2]));
    if(argc==4 && !strcmp(argv[3],"read")) {
        CHECK(Save_Load(0,1000));CHECK(GameCanSave());CHECK(Save_Write(5,"restart",true));End();return 0;
    }
    if(argc==4 && !strcmp(argv[3],"episodes")) {
        for(unsigned ep=0;ep<6;++ep) {
            CHECK(GameStartSelected(0,ep,2));Step(1,20);
            CHECK(Save_Write(0,"episode checkpoint",true));Step(21,40);
            CHECK(Save_Write(1,"continuous",true));unsigned rng=US_RndIndex();
            CHECK(Save_Load(0,20));CHECK((unsigned)gamestate.episode==ep);
            Step(21,40);CHECK(US_RndIndex()==rng);CHECK(Save_Write(2,"restored",true));
            char first[4096],second[4096];(void)snprintf(first,sizeof(first),"%s/slot2.w3s",argv[2]);
            (void)snprintf(second,sizeof(second),"%s/slot3.w3s",argv[2]);
            FILE *a=fopen(first,"rb"),*b=fopen(second,"rb");CHECK(a && b);
            CHECK(!fseek(a,96,SEEK_SET) && !fseek(b,96,SEEK_SET));
            int x,y;do { x=fgetc(a);y=fgetc(b);CHECK(x==y); } while(x!=EOF);
            CHECK(!ferror(a) && !ferror(b));CHECK(!fclose(a) && !fclose(b));
            IN_ClearKeysDown();playstate=ex_completed;CHECK(GameStep(61) && GameIntermission());
            IN_KeyEvent(sc_Enter,true);CHECK(GameStep(62));
            if(GameIntermission()) { IN_ClearKeysDown();IN_KeyEvent(sc_Enter,true);CHECK(GameStep(63)); }
            CHECK(!GameIntermission() && gamestate.mapon==1 && (unsigned)gamestate.episode==ep);
        }
        End();return 0;
    }
    Start();CHECK(GameCanSave());
    if(argc==4 && !strcmp(argv[3],"scenario")) { Scenario();End();return 0; }
    if(argc==4 && !strcmp(argv[3],"respawn")) {
        TakeDamage(100,NULL);CHECK(!Save_Write(0,"dead",true));
        unsigned t=0;while(gamestate.died && t<1000) CHECK(GameStep(++t));
        CHECK(!gamestate.died && gamestate.lives==2);CHECK(Save_Write(0,"respawn",true));
        CHECK(Save_Load(0,t));CHECK(gamestate.health==100 && gamestate.ammo==8 && gamestate.lives==2);
        End();return 0;
    }
    if(argc==4 && !strcmp(argv[3],"audio")) {
        SD_SetDigitizedMode(true);Start();CHECK(SD_Startup(48000));
        CHECK(Save_Write(0,"with audio",true));SD_SetVolumes(22,11);SD_SetEnabled(true,false);
        uint64_t generation=SD_OutputGeneration();CHECK(Save_Load(0,0));
        unsigned m,e;bool mon,eon;SD_GetSettings(&m,&e,&mon,&eon);sd_status audio;SD_GetStatus(&audio);
        CHECK(m==22 && e==11 && mon && !eon && audio.sound==-1 && audio.digitized==-1);
        CHECK(audio.music==SD_LevelMusic(0,0) && SD_OutputGeneration()>generation && s_schabbdie2.tictime==140);
        End();return 0;
    }
    if(argc==4 && !strcmp(argv[3],"reject")) {
        Step(1,10);CHECK(Save_Write(4,"before rejection",true));unsigned rng=US_RndIndex();
        CHECK(!Save_Load(0,500));CHECK(GameCanSave() && US_RndIndex()==rng);
        CHECK(Save_Write(5,"after rejection",true));End();return 0;
    }
    if(argc==4 && !strcmp(argv[3],"writefail")) { CHECK(!Save_Write(0,"failed replacement",true));End();return 0; }
    if(argc==4 && !strncmp(argv[3],"create",6)) { bool ok=Save_Write(0,argv[3],false);End();return ok?0:2; }

    for(unsigned i=0;i<SAVE_SLOTS;++i) {
        save_info info;Save_ReadInfo(i,&info);CHECK(info.status==SAVE_EMPTY);
        CHECK(Save_Write(i,"test save",false));Save_ReadInfo(i,&info);CHECK(info.status==SAVE_VALID && !strcmp(info.description,"test save"));
        CHECK(!Save_Write(i,"not approved",false));CHECK(Save_Write(i,"updated",true));
    }
    CHECK(!Save_Write(6,"invalid",true));CHECK(!Save_Write(0,"",true));
    /* Reuse an interior hole and preserve actor ordering/reservations. */
    if(player.next && player.next->next) { objtype *gone=player.next;RemoveActor(gone);CHECK(GetNewActor()==gone);gone->obclass=inertobj;gone->state=Save_State(1);gone->x=gone->y=20*TILEGLOBAL;gone->tilex=gone->tiley=20; }
    Step(1,50);CHECK(Save_Write(0,"midgame",true));
    Step(51,80);CHECK(Save_Write(1,"continuous",true));unsigned rng=US_RndIndex();
    CHECK(Save_Load(0,50));CHECK(GameCanSave());Step(51,80);CHECK(US_RndIndex()==rng);CHECK(Save_Write(2,"restored",true));
    /* The Python oracle compares the complete portable payloads. */
    for(unsigned id=0;id<Save_StateCount();++id) CHECK(Save_StateID(Save_State(id))==id);
    CHECK(Save_State(99999)==NULL);
    End();puts("Save slot metadata, round trips, state IDs and deterministic continuation passed.");return 0;
}
