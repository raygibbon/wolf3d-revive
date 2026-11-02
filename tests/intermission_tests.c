/* SPDX-License-Identifier: GPL-2.0-only */
#include "engine/WL_SAVE.h"
#include "engine/ID_CA.h"
#include "engine/ID_SD.h"
#include "engine/ID_US_1.h"
#include "engine/ID_VL.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(x) do { if(!(x)) { fprintf(stderr,"%d: %s (%s)\n",__LINE__,#x,CA_Error());exit(1); } } while(0)
static soundnames events[256];static uint64_t times[256],now;static unsigned count;
static void Sound(soundnames sound,const objtype *actor) { (void)actor;CHECK(count<256);events[count]=sound;times[count++]=now; }
int main(int argc,char **argv)
{
    CHECK(argc==2);MM_Startup();CHECK(CA_Startup(argv[1]));CHECK(VL_SetPalette(gamepal,768));CHECK(GameStart(0));
    /* Invented AL sounds last one second (140 samples), even without SDL. */
    CHECK(SD_SoundTicks(ENDBONUS1SND)==70);SetGameSoundHook(Sound);
    levelratio_t r={100,0,30,60,90,25000};unsigned rng=US_RndIndex();int score=gamestate.score;
    CHECK(StartLevelIntermission(&r,0));CHECK(count==1 && events[0]==ENDBONUS1SND && LevelIntermissionCounting());
    uint8_t first[VL_PIXELS],second[VL_PIXELS];memcpy(first,framebuffer,sizeof(first));
    bool leave=false;now=10;CHECK(StepLevelIntermission(now,false,&leave));CHECK(!memcmp(first,framebuffer,sizeof(first)));
    now=11;CHECK(StepLevelIntermission(now,false,&leave));CHECK(memcmp(first,framebuffer,sizeof(first)));
    memcpy(second,framebuffer,sizeof(second));now=46;CHECK(StepLevelIntermission(now,false,&leave));CHECK(!memcmp(second,framebuffer,sizeof(second)));
    now=47;CHECK(StepLevelIntermission(now,false,&leave));CHECK(!memcmp(first,framebuffer,sizeof(first)));
    now=69;CHECK(StepLevelIntermission(now,false,&leave) && count==1 && gamestate.score==score);
    now=70;CHECK(StepLevelIntermission(now,false,&leave) && count==2 && events[1]==ENDBONUS2SND);
    for(now=71;now<5000 && LevelIntermissionCounting();++now) CHECK(StepLevelIntermission(now,false,&leave));
    CHECK(!LevelIntermissionCounting() && gamestate.score==score+25000 && US_RndIndex()==rng && !leave);
    CHECK(count==21); /* time 2, kills 11+perfect, secrets 1+none, treasure 4+end */
    unsigned perfect=0,none=0;
    for(unsigned i=0;i<count;++i) { perfect+=events[i]==PERCENT100SND;none+=events[i]==NOBONUSSND;if(i) CHECK(times[i]>=times[i-1]); }
    CHECK(perfect==1 && none==1 && events[count-1]==ENDBONUS2SND);
    CHECK(StepLevelIntermission(now,true,&leave) && leave && gamestate.score==score+25000);
    count=0;score=gamestate.score;now=6000;CHECK(StartLevelIntermission(&r,now));
    CHECK(StepLevelIntermission(now+1,true,&leave) && !leave && !LevelIntermissionCounting());
    CHECK(gamestate.score==score+25000 && US_RndIndex()==rng);
    CHECK(StepLevelIntermission(now+2,true,&leave) && leave && gamestate.score==score+25000);
    SetGameSoundHook(NULL);GameStop();CA_Shutdown();MM_Shutdown();CHECK(!MM_TotalBytes());
    puts("Original intermission counts, sound waits, skip/confirm, single award and RNG passed.");return 0;
}
