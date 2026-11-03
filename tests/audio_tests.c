/* SPDX-License-Identifier: GPL-2.0-only */
#include "engine/ID_SD.h"
#include "engine/ID_FILE.h"
#include "engine/ID_CA.h"
#include "engine/ID_VL.h"
#include "engine/ID_IN.h"
#include "engine/ID_US_1.h"
#include "engine/WL_DEF.h"
#include "engine/WL_MENU.h"
#include "platform/sdl_audio.h"
#include <SDL3/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(x) do { if(!(x)) { fprintf(stderr,"%d: %s (%s; %s)\n",__LINE__,#x,CA_Error(),Audio_Error());exit(1); } } while(0)
static int16_t pcm[48000*2],other[48000*2];
static void Reset(unsigned rate)
{
    SD_SetEnabled(true,true);SD_SetVolumes(70,85);SD_SetDigitizedMode(true);
    CHECK(SD_Startup(rate));
}
static long long Energy(const int16_t *p,size_t frames,unsigned channel)
{
    long long sum=0;for(size_t i=0;i<frames;++i) sum+=abs(p[i*2+channel]);return sum;
}
static void Timing(void)
{
    for(unsigned rate=44100;rate<=48000;rate+=3900) {
        Reset(rate);SD_StartMusic(0);SD_Render(pcm,rate);
        sd_status s;SD_GetStatus(&s);CHECK(s.ticks==700 && s.loops==1 && s.frames==rate);
        CHECK(s.registers[0xb1]==0 && s.writes>12 && Energy(pcm,rate,0)>10000);
        uint64_t writes=s.writes;SD_Render(other,1);SD_GetStatus(&s);
        CHECK(s.ticks==701 && s.loops==1 && s.writes==writes+12 && s.registers[0xb1]==0x31);
        /* One large request and arbitrarily split requests must be identical. */
        Reset(rate);SD_StartMusic(0);SD_Render(other,1);SD_Render(other+2,17);SD_Render(other+36,rate-18);
        CHECK(!memcmp(pcm,other,rate*2*sizeof(*pcm)));
        SD_StartMusic(-1);SD_GetStatus(&s);CHECK(s.registers[0xb1]==0 && s.music==-1);
    }
}
static void Effects(void)
{
    Reset(48000);SD_SetDigitizedMode(false);SD_StartMusic(0);
    CHECK(SD_PlaySound(MISSILEHITSND,0,0));CHECK(!SD_PlaySound(HITWALLSND,0,0));
    CHECK(SD_PlaySound(MISSILEHITSND,0,0)); /* Equal priority interrupts. */
    SD_Render(pcm,48000);sd_status s;SD_GetStatus(&s);
    CHECK(s.sound==-1 && !s.sound_priority && s.registers[0xb0]==0);
    CHECK(s.loops==1 && Energy(pcm,48000,0)>10000);
    Reset(48000);CHECK(SD_PlaySound(MISSILEHITSND,0,0));SD_Render(pcm,4800);
    SD_GetStatus(&s);CHECK(s.registers[0xb0]==0x30 && s.registers[0xc0]==0);
    /* One chip: effect channel zero and IMF channel one coexist. */
    SD_StartMusic(0);SD_Render(pcm,480);SD_GetStatus(&s);
    CHECK(s.registers[0xb0]==0x30 && s.registers[0xb1]==0x31);
    SD_SetDigitizedMode(true);CHECK(SD_PlaySound(ROSESND,0,15));
    CHECK(!SD_PlaySound(HALTSND,0,0));SD_GetStatus(&s);
    CHECK(s.digitized==45 && s.digi_priority==85 && s.sound_priority==87);
    SD_StopSound();CHECK(!SD_PlaySound((soundnames)LASTSOUND,0,0));
    Reset(48000);SD_SetDigitizedMode(false);CHECK(SD_PlaySound(MISSILEHITSND,0,0));
    SD_StartMusic(26);SD_Render(pcm,276);SD_GetStatus(&s);
    /* At tick 4 the effect writes first; IMF overwrites the SAME chip channel. */
    CHECK(s.registers[0xb0]==0x2c && s.registers[0xa0]==37);
    /* Exact SB time constant: 142 source samples last 142*142 microseconds. */
    Reset(48000);CHECK(SD_PlaySound(HALTSND,0,15));SD_Render(pcm,968);
    CHECK(Energy(pcm,968,0)>100000 && !Energy(pcm,968,1));
    SD_GetStatus(&s);CHECK(s.digitized==0);SD_Render(pcm,1);SD_GetStatus(&s);CHECK(s.digitized==-1);
    CHECK(SD_PlaySound(DEATHSCREAM2SND,0,0));SD_GetStatus(&s);CHECK(s.digitized==13);
    CHECK(SD_PlaySound(DEATHSCREAM3SND,0,0));SD_GetStatus(&s);CHECK(s.digitized==13);
}
static void Volume(void)
{
    Reset(48000);SD_StartMusic(0);SD_SetVolumes(0,0);SD_Render(pcm,48000);
    CHECK(!Energy(pcm,48000,0) && !Energy(pcm,48000,1));
    SD_SetVolumes(100,100);SD_StartMusic(0);SD_Render(pcm,48000);CHECK(Energy(pcm,48000,0)>10000);
    SD_SetEnabled(false,false);SD_Render(pcm,48000);SD_Render(pcm,48000);
    CHECK(!Energy(pcm,48000,0) && !SD_PlaySound(HALTSND,0,0));
    SD_SetEnabled(true,true);SD_SetVolumes(0,50);SD_StopSound();
    CHECK(SD_PlaySound(HALTSND,0,0));SD_Render(pcm,968);long long half=Energy(pcm,968,0);
    SD_SetVolumes(0,100);CHECK(SD_PlaySound(HALTSND,0,0));SD_Render(pcm,968);
    CHECK(Energy(pcm,968,0)==2*half);
}
static void PositionAndRNG(void)
{
    BuildTables();player.x=player.y=20*TILEGLOBAL;player.angle=0;
    unsigned l,r;GameSoundPosition(player.x,player.y,&l,&r);CHECK(!l && !r);
    GameSoundPosition(player.x,player.y+4*TILEGLOBAL,&l,&r);CHECK(l>r);
    GameSoundPosition(player.x,player.y-4*TILEGLOBAL,&l,&r);CHECK(r>l);
    player.angle=180;GameSoundPosition(player.x,player.y+4*TILEGLOBAL,&l,&r);CHECK(r>l);
    GameSoundPosition(0,0,&l,&r);CHECK(l==8 && r==8);
    US_InitRndT(20);int expected=US_RndT();US_InitRndT(20);
    Reset(48000);SD_StartMusic(0);GameSound(HALTSND,&player);SD_Render(pcm,48000);CHECK(US_RndT()==expected);
    SD_SetDigitizedMode(true);SD_SetEnabled(false,false);CHECK(SD_DigitizedMode());SD_Shutdown();CHECK(SD_DigitizedMode());
}
static void OwnedTracks(void)
{
    memptr head=NULL,archive=NULL;size_t hs,as;
    CHECK(CA_ReadDataFile("AUDIOHED.WL6",&head,&hs));CHECK(CA_ReadDataFile("AUDIOT.WL6",&archive,&as));
    CHECK(hs>=289*4);
    for(unsigned song=0;song<27;++song) {
        const uint8_t *s=(const uint8_t *)archive+CA_LE32((const uint8_t *)head+(261+song)*4);
        size_t length=CA_LE16(s);uint64_t ticks=0;
        for(size_t pos=0;pos+4<length;pos+=4) ticks+=CA_LE16(s+2+pos+2);
        Reset(48000);SD_StartMusic((int)song);
        uint64_t remaining=(ticks*48000+699)/700;
        while(remaining) {
            size_t frames=(size_t)(remaining>48000?48000:remaining);SD_Render(pcm,frames);remaining-=frames;
        }
        sd_status status;SD_GetStatus(&status);CHECK(!status.loops);
        SD_Render(pcm,1);SD_GetStatus(&status);CHECK(status.loops==1);
    }
    (void)MM_FreePtr(&head);(void)MM_FreePtr(&archive);
}
static void GameplayIndependence(void)
{
    gametype saved={0};objtype actors[MAXACTORS]={{0}},savedplayer={0};unsigned count=0;int rng=0;
    for(unsigned enabled=0;enabled<2;++enabled) {
        SD_SetDigitizedMode(true);
        if(enabled) { CHECK(SD_Startup(48000));SD_SetEnabled(true,true); }
        else SD_Shutdown();
        CHECK(VL_SetPalette(gamepal,768));CHECK(GameStartSelected(0,0,2));
        for(unsigned tic=1;tic<=210;++tic) {
            IN_KeyEvent(0x1d,(tic%70)<35);CHECK(GameStep(tic));SD_Render(pcm,48000/70);
        }
        if(!enabled) {
            saved=gamestate;count=objcount;memcpy(actors,objlist,sizeof(actors));savedplayer=player;rng=US_RndT();
        } else {
            CHECK(!memcmp(&saved,&gamestate,sizeof(saved)) && count==objcount);
            CHECK(!memcmp(actors,objlist,sizeof(actors)) && !memcmp(&savedplayer,&player,sizeof(player)));
            CHECK(US_RndT()==rng);
        }
        GameStop();IN_ClearKeysDown();SD_Shutdown();
    }
}
static void SettingsMenu(void)
{
    SD_SetEnabled(true,true);SD_SetVolumes(70,85);CHECK(BeginSoundSettings());
    IN_KeyEvent(sc_Enter,true);IN_KeyEvent(sc_Enter,false);CHECK(HandleSoundSettings()==0);
    unsigned m,e;bool mon,eon;SD_GetSettings(&m,&e,&mon,&eon);CHECK(mon && !eon);
    /* Navigate past the supported AdLib row into the volume subpage. */
    for(unsigned i=0;i<2;++i) { IN_KeyEvent(sc_DownArrow,true);IN_KeyEvent(sc_DownArrow,false);CHECK(HandleSoundSettings()==0); }
    IN_KeyEvent(sc_Enter,true);IN_KeyEvent(sc_Enter,false);CHECK(HandleSoundSettings()==0);
    IN_KeyEvent(sc_LeftArrow,true);IN_KeyEvent(sc_LeftArrow,false);CHECK(HandleSoundSettings()==0);
    SD_GetSettings(&m,&e,&mon,&eon);CHECK(m==65 && e==85);
    IN_KeyEvent(sc_Escape,true);IN_KeyEvent(sc_Escape,false);CHECK(HandleSoundSettings()==0);
    IN_KeyEvent(sc_DownArrow,true);IN_KeyEvent(sc_DownArrow,false);CHECK(HandleSoundSettings()==0);
    IN_KeyEvent(sc_Enter,true);IN_KeyEvent(sc_Enter,false);CHECK(HandleSoundSettings()==0);
    SD_GetSettings(&m,&e,&mon,&eon);CHECK(!mon && !eon);
    IN_KeyEvent(sc_Escape,true);IN_KeyEvent(sc_Escape,false);CHECK(HandleSoundSettings()==-1);
    IN_ClearKeysDown();
}
static void Device(bool listen)
{
    for(unsigned cycle=0;cycle<(listen?1U:8U);++cycle) {
        SD_SetEnabled(true,true);SD_SetDigitizedMode(true);CHECK(Audio_Startup());CHECK(Audio_Active());
        SD_StartMusic(listen?7:0);uint64_t start=SDL_GetTicks();
        do { Audio_Pump();SDL_Delay(4); } while(SDL_GetTicks()-start<(listen?6000U:40U));
        SD_GetStatus(&(sd_status){0});
        if(listen) for(unsigned sound=0;sound<4;++sound) {
            static const soundnames sounds[]={ATKPISTOLSND,HALTSND,GETAMMOSND,YEAHSND};
            SD_StopSound();CHECK(SD_PlaySound(sounds[sound],0,0));start=SDL_GetTicks();
            do { Audio_Pump();SDL_Delay(4); } while(SDL_GetTicks()-start<1500);
        }
        sd_status s;SD_GetStatus(&s);CHECK(s.frames>0 && s.writes>0);
        Audio_Shutdown();Audio_Shutdown();CHECK(!Audio_Active() && !SD_Ready());
    }
    (void)SDL_setenv_unsafe("SDL_AUDIODRIVER","nonexistent-wolf-test",1);
    CHECK(!Audio_Startup());CHECK(!Audio_Active() && !SD_Ready() && *Audio_Error());Audio_Shutdown();
}
int main(int argc,char **argv)
{
    CHECK(argc>=2);MM_Startup();
    if(!CA_Startup(argv[1])) {
        CHECK(argc==3 && !strcmp(argv[2],"invalid"));
        CA_Shutdown();MM_Shutdown();CHECK(!MM_TotalBytes());return 0;
    }
    if(argc==3 && !strcmp(argv[2],"invalid")) {
        CHECK(!SD_Startup(48000));CHECK(!SD_Ready());SD_Shutdown();
    } else if(argc==3 && !strcmp(argv[2],"listen")) Device(true);
    else if(argc==3 && !strcmp(argv[2],"gameplay")) GameplayIndependence();
    else if(argc==3 && !strcmp(argv[2],"owned")) {
        OwnedTracks();Reset(48000);
        for(int song=0;song<27;++song) { SD_StartMusic(song);SD_Render(pcm,48000);CHECK(Energy(pcm,48000,0)>0); }
        for(unsigned sound=0;sound<LASTSOUND;++sound) { SD_StopSound();CHECK(SD_PlaySound((soundnames)sound,0,0));SD_Render(pcm,4800); }
        Device(false);
    } else { Timing();Effects();Volume();PositionAndRNG();SettingsMenu();Device(false); }
    Audio_Shutdown();CA_Shutdown();MM_Shutdown();CHECK(!MM_TotalBytes());SDL_Quit();
    puts("Audio resources, scheduling, PCM, priorities, position, volume and lifecycle passed.");return 0;
}
